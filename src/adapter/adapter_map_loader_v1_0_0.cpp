#include <cpptoml/cpptoml.h>
#include <ninja_clown/api.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "adapter/adapter_mapfile_constants.hpp"
#include "facing_dir.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"
#include "utils/visitor.hpp"
#include "view/game/game_viewer.hpp"
#include "view/game/map.hpp"
#include "view/game/map_viewer.hpp"
#include "view/game/mob.hpp"
#include "view/game/object.hpp"
#include "view/view.hpp"

using fmt::literals::operator""_a;
namespace values = adapter::map_file::values;
namespace keys   = adapter::map_file::keys;

namespace {

template <typename T>
using usmap = std::unordered_map<std::string, T>;

struct point {
	std::size_t x, y;
};

struct mob_definition { };

struct mob {
	point pos;
	float facing;
	unsigned int hp;
	model::tick_t attack_delay;
	model::tick_t throw_delay;
	adapter::entity_prop::behaviour behaviour;
};

enum class activator_type {
	NONE,
	BUTTON,
	INDUCTION_LOOP,
	INFRARED_LASER,
};

struct activator {
	point pos;
	model::tick_t delay;
	model::tick_t refire_after;
	bool refire_repeat;
	model::tick_t activation_difficulty;
	activator_type type;
	std::vector<size_t> target_tiles;
	std::string name;
};

struct gate {
	std::string name;
	point pos;
	bool closed;
};

struct autoshooter {
	std::string name;
	point pos;
	unsigned int firing_rate;
	float facing;
};

usmap<adapter::entity_prop::behaviour> globstr_to_behaviour;
usmap<activator_type> globstr_to_activator_type;

template <typename T, typename... Args>
bool try_get_silent(const std::shared_ptr<cpptoml::table> &table, const char *key, T &value) {
	cpptoml::option<T> maybe_value = table->get_qualified_as<T>(key);
	if (maybe_value) {
		value = *maybe_value;
	}
	return maybe_value.operator bool();
}

template <typename T, typename... Args>
bool try_get(const std::shared_ptr<cpptoml::table> &table, const char *key, T &value, std::string_view fmt, Args &&...error_args) {
	if (!try_get_silent(table, key, value)) {
		spdlog::error(fmt, std::forward<Args>(error_args)...);
		return false;
	}
	return true;
}

void init_maps() {
	using adapter::entity_prop::behaviour;

	if (globstr_to_activator_type.empty()) {
		globstr_to_behaviour.emplace(values::none, behaviour{behaviour::bhvr::unsupported});
		globstr_to_behaviour.emplace(values::harmless, behaviour{behaviour::bhvr::harmless});
		globstr_to_behaviour.emplace(values::patrol, behaviour{behaviour::bhvr::patrol});
		globstr_to_behaviour.emplace(values::aggressive, behaviour{behaviour::bhvr::aggressive});
		globstr_to_behaviour.emplace(values::dll, behaviour{behaviour::bhvr::dll});
		globstr_to_activator_type.emplace(values::button, activator_type::BUTTON);
		globstr_to_activator_type.emplace(values::induction_loop, activator_type::INDUCTION_LOOP);
		globstr_to_activator_type.emplace(values::infrared_laser, activator_type::INFRARED_LASER);
	}
}

[[nodiscard]] std::vector<mob> load_mobs_spawns(const std::shared_ptr<cpptoml::table_array> &tables, std::string_view map) {
	std::vector<mob> loaded_mobs;
	for (const auto &mob : *tables) {
		auto try_get = [&map, &mob](const char *key, auto &value) -> bool {
			return ::try_get(mob, key, value,
			                 utils::resource_manager::instance().log_for("adapter_map_loader_v1_0_0.mob_spawn_missing_component"),
			                 "map"_a = map, "key"_a = key);
		};

		std::size_t x_pos{};
		if (!try_get(keys::x_pos, x_pos)) {
			return {};
		}

		std::size_t y_pos{};
		if (!try_get(keys::y_pos, y_pos)) {
			return {};
		}

		double facing{};
		if (!try_get(keys::facing, facing)) {
			return {};
		}

		unsigned int hp{};
		if (!try_get(keys::hp, hp)) {
			return {};
		}

		model::tick_t attack_delay{};
		if (!try_get_silent(mob, keys::attack_delay, attack_delay)) {
			attack_delay = model::component::default_attack_delay;
		}

		model::tick_t throw_delay{};
		if (!try_get_silent(mob, keys::throw_delay, throw_delay)) {
			throw_delay = model::component::default_throw_delay;
		}


		using adapter::entity_prop::behaviour;

		behaviour bhvr{};
		const cpptoml::option<std::string> behaviour_str = mob->get_as<std::string>(keys::behaviour);
		if (auto it = globstr_to_behaviour.find(behaviour_str.value_or("")); it != globstr_to_behaviour.end()) {
			bhvr = it->second;
		}
		else {
			utils::log::error(utils::resource_manager::instance().log_for("adapter_map_loader_v1_0_0.mob_spawn_missing_component"),
			                  "map"_a = map, "key"_a = keys::behaviour);
			return {};
		}

		loaded_mobs.emplace_back(::mob{point{x_pos, y_pos}, static_cast<float>(facing), hp, attack_delay, throw_delay, bhvr});
	}
	return loaded_mobs;
}

[[nodiscard]] std::optional<std::vector<std::variant<activator, gate, autoshooter>>>
load_actors(const std::shared_ptr<cpptoml::table_array> &tables, std::string_view map) {
	assert(!globstr_to_activator_type.empty()); // NOLINT

	auto error = [&map](const char *key, auto &&...vals) {
		utils::log::error(std::string("adapter_map_loader_v1_0_0.") + key, "map"_a = map, std::forward<decltype(vals)>(vals)...);
	};

	std::vector<std::variant<activator, gate, autoshooter>> actors;

	std::unordered_map<std::string, size_t> actionable_handles;

	for (const auto &actor : *tables) {
		auto type = actor->get_as<std::string>(keys::type);
		if (!type) {
			error("actor_with_unknown_type", "key"_a = keys::type);
			return {};
		}

		auto try_get = [&actor, &type, &map](const char *key, auto &value, bool silent = false) -> bool {
			if (silent) {
				return try_get_silent(actor, key, value);
			}
			else {
				return ::try_get(actor, key, value,
				                 utils::resource_manager::instance().log_for("adapter_map_loader_v1_0_0.actor_missing_component"),
				                 "key"_a = key, "type"_a = *type, "map"_a = map);
			}
		};

		std::size_t x_pos{};
		if (!try_get(keys::x_pos, x_pos)) {
			return {};
		}

		std::size_t y_pos{};
		if (!try_get(keys::y_pos, y_pos)) {
			return {};
		}

		if (*type == values::autoshooter) {
			unsigned int firing_rate{};
			if (!try_get(keys::firing_rate, firing_rate)) {
				return {};
			}

			double facing{};
			if (!try_get(keys::facing, facing)) {
				return {};
			}

			std::string name;
			if (!try_get(keys::name, name)) {
				return {};
			}

			auto insert_result = actionable_handles.insert_or_assign(name, actors.size());
			if (!insert_result.second) {
				utils::log::warn("adapter_map_loader_v1_0_0.actor_duplication", "actor"_a = name, "map"_a = map);
			}
			actors.emplace_back(autoshooter{name, point{x_pos, y_pos}, firing_rate, static_cast<float>(facing)});
		}
		else if (*type == values::gate) {

			bool closed{};
			if (!try_get(keys::closed, closed)) {
				return {};
			}

			std::string name;
			if (!try_get(keys::name, name)) {
				return {};
			}

			auto insert_result = actionable_handles.insert_or_assign(name, actors.size());
			if (!insert_result.second) {
				utils::log::warn("adapter_map_loader_v1_0_0.actor_duplication", "actor"_a = name, "map"_a = map);
			}
			actors.emplace_back(gate{name, point{x_pos, y_pos}, closed});
		}
		else {
			activator activator;
			activator.pos = {x_pos, y_pos};

			auto parsed_type = globstr_to_activator_type.find(*type);
			if (parsed_type == globstr_to_activator_type.end()) {
				error("actor_with_unrecognized_type", "type"_a = *type, "key"_a = keys::type);
				return {};
			}
			activator.type = parsed_type->second;

			if (!try_get(keys::name, activator.name)) {
				return {};
			}

			activator.refire_after = std::numeric_limits<decltype(activator.refire_after)>::max();
			if (!try_get(keys::refire_after, activator.refire_after, true)) {
				try_get(keys::duration, activator.refire_after, true);
			}

			activator.refire_repeat = false;
			if (!try_get(keys::refire_repeat, activator.refire_repeat, true)) {
				try_get(keys::duration, activator.refire_repeat, true);
			}

			activator.activation_difficulty = model::default_activation_difficulty;
			try_get(keys::activation_difficulty, activator.activation_difficulty, true);

			activator.delay = model::default_activation_delay;
			try_get(keys::delay, activator.delay, true);

			auto targets = actor->get_array_of<std::string>(keys::acts_on_table);
			if (!targets) {
				error("actor_with_no_target", "type"_a = *type, "key"_a = keys::acts_on_table);
				return {};
			}
			for (const std::string &target : *targets) {

				auto it = actionable_handles.find(target);
				if (it == actionable_handles.end()) {
					error("actor_with_unknown_target", "type"_a = *type, "target"_a = target);
					return {};
				}

				activator.target_tiles.push_back(it->second);
			}

			actors.emplace_back(std::move(activator));
		}
	}

	return actors;
}

} // namespace

bool adapter::adapter::load_map_v1_0_0(const std::shared_ptr<cpptoml::table> &map_file, std::string_view map) noexcept {
	init_maps();

	auto error = [&map](const char *key, auto &&...vals) {
		utils::log::error(std::string("adapter_map_loader_v1_0_0.") + key, "map"_a = map, std::forward<decltype(vals)>(vals)...);
	};

	view::map_viewer map_viewer{m_state};

	{

		auto mobs_spawns_toml = map_file->get_table_array_qualified(keys::mobs_spawn_table);
		auto actors_toml      = map_file->get_table_array_qualified(keys::actors_spawn_table);
		auto target_toml      = map_file->get_table_qualified(keys::target);
		auto map_layout_toml  = map_file->get_qualified_array_of<std::string>(keys::map_layout);

		if (!mobs_spawns_toml || !map_layout_toml || !target_toml) {
			if (!mobs_spawns_toml) {
				error("missing_mob_placings");
			}
			if (!map_layout_toml) {
				error("missing_map_layout");
			}
			if (!target_toml) {
				error("missing_target");
			}
			return false;
		}

		const std::vector<mob> mobs = load_mobs_spawns(mobs_spawns_toml, map);
		if (mobs.empty()) {
			error("bad_mob_spawns", "map"_a = map);
			return false;
		}
		if (mobs.size() > model::cst::max_entities) {
			error("too_many_spawns", "map"_a = map);
			return false;
		}

		std::vector<std::variant<activator, gate, autoshooter>> actors;
		if (actors_toml) {
			auto maybe_actors = load_actors(actors_toml, map);
			if (!maybe_actors) {
				return false;
			}
			actors = std::move(*maybe_actors);
		}

		size_t map_height = map_layout_toml->size();
		if (map_height == 0) {
			error("empty_map", "map"_a = map);
			return false;
		}
		size_t map_width = map_layout_toml->begin()->size();
		if (map_width == 0) {
			error("empty_map", "map"_a = map);
			return false;
		}

		auto map_viewer_omap = map_viewer.m_overmap.acquire();

		model::world &world = state::access<adapter>::model(m_state).world;

		std::vector<std::vector<view::cell>> view_map{map_width, {map_height, {view::cell::abyss}}};
		world.map.resize(map_width, map_height);

		unsigned int line_idx{0};
		for (const std::string &line : *map_layout_toml) {

			if (line.size() != map_width) {
				utils::log::error("adapter_map_loader_v1_0_0.bad_map_size", "map"_a = map, "line"_a = line_idx + 1);
				return false;
			}

			for (size_t column_idx = 0; column_idx < map_width; ++column_idx) {
				switch (line[column_idx]) {
					case '#':
						world.map[column_idx][line_idx].type = model::cell_type::CHASM;
						view_map[column_idx][line_idx]       = view::cell::abyss;
						break;
					case ' ':
						world.map[column_idx][line_idx].type = model::cell_type::GROUND;
						view_map[column_idx][line_idx]       = view::cell::concrete_tile;
						break;
					case '~':
						world.map[column_idx][line_idx].type = model::cell_type::GROUND;
						view_map[column_idx][line_idx]       = view::cell::iron_tile;
						break;
					default:
						error("bad_map_char", "char"_a = line[column_idx], "line"_a = line_idx + 1, "column"_a = column_idx + 1);
						return false;
				}
			}

			++line_idx;
		}


		unsigned int target_x{}, target_y{};
		try_get(target_toml, map_file::keys::x_pos, target_x, "adapter_map_loader_v1_0_0.missing_target", "map"_a = map);
		try_get(target_toml, map_file::keys::y_pos, target_y, "adapter_map_loader_v1_0_0.missing_target", "map"_a = map);

		world.target_tile = {target_x, target_y};

		view::object obj;
		obj.set_id(utils::resources_type::object_id::target);
		obj.set_pos(world.target_tile.x * model::cst::cell_width, world.target_tile.y * model::cst::cell_height);
		obj.reveal();
		m_target_handle = map_viewer_omap->add_object(std::move(obj));


		// TODO externaliser vers le fichier de la carte + ajouter une option de mise à l’échelle des objets graphiques
		const float DEFAULT_HITBOX_HALF_WIDTH  = 0.25f;
		const float DEFAULT_HITBOX_HALF_HEIGHT = 0.25f;

		size_t model_entity_handle = 0;
		for (const mob &mob : mobs) {
			const float CENTER_X = static_cast<float>(mob.pos.x) * model::cst::cell_width + model::cst::cell_width / 2.f;
			const float CENTER_Y = static_cast<float>(mob.pos.y) * model::cst::cell_height + model::cst::cell_height / 2.f;

			using bhvr = entity_prop::behaviour::bhvr;
			switch (mob.behaviour.val) {
				case bhvr::harmless:
					world.components.metadata[model_entity_handle].kind = ninja_api::nnj_entity_kind::EK_HARMLESS;
					break;
				case bhvr::unsupported:
					world.components.metadata[model_entity_handle].kind = ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY;
					break;
				case bhvr::dll:
					world.components.metadata[model_entity_handle].kind = ninja_api::nnj_entity_kind::EK_DLL;
					break;
				case bhvr::patrol:
					world.components.metadata[model_entity_handle].kind = ninja_api::nnj_entity_kind::EK_PATROL;
					break;
				case bhvr::aggressive:
					world.components.metadata[model_entity_handle].kind = ninja_api::nnj_entity_kind::EK_AGGRESSIVE;
					break;
			}

			utils::resources_type::mob_id resource_id = view::view::mob_id_from_behaviour(mob.behaviour);

			world.components.health[model_entity_handle]      = {static_cast<std::uint8_t>(mob.hp)};
			world.components.hitbox[model_entity_handle]      = {CENTER_X, CENTER_Y, DEFAULT_HITBOX_HALF_WIDTH, DEFAULT_HITBOX_HALF_HEIGHT};
			world.components.hitbox[model_entity_handle]->rad = mob.facing;
			world.components.properties[model_entity_handle].throw_delay  = mob.throw_delay;
			world.components.properties[model_entity_handle].attack_delay = mob.attack_delay;

			model::component::hitbox &hitbox = *world.components.hitbox[model_entity_handle];

			view::mob m{};
			m.set_mob_id(resource_id);
			m.set_direction(view::facing_direction::from_angle(mob.facing));
			m.set_pos(hitbox.center.x, hitbox.center.y);

			view_handle view_handle = map_viewer_omap->add_mob(std::move(m));
			model_handle model_handle{model_entity_handle, model_handle::ENTITY};
			m_model2view[model_handle] = view_handle;
			m_view2model[view_handle]  = model_handle;

			++model_entity_handle;
		}

		for (const std::variant<activator, gate, autoshooter> &actor : actors) {

			utils::visitor visitor{
			  [&](const activator &activator) {
				  const float TOPLEFT_X = static_cast<float>(activator.pos.x) * model::cst::cell_width;
				  const float TOPLEFT_Y = static_cast<float>(activator.pos.y) * model::cst::cell_height;

				  world.map[activator.pos.x][activator.pos.y].interaction_handle = {world.interactions.size()};

				  view::object o{};
				  o.set_pos(TOPLEFT_X, TOPLEFT_Y);

				  switch (activator.type) {

					  case activator_type::BUTTON:
						  world.interactions.push_back(
						    {model::interaction_kind::LIGHT_MANUAL, model::interactable_kind::BUTTON, world.activators.size()});
						  o.set_id(utils::resources_type::object_id::button);
						  break;
					  case activator_type::INDUCTION_LOOP:
						  world.interactions.push_back(
						    {model::interaction_kind::LIGHT_MIDAIR, model::interactable_kind::INDUCTION_LOOP, world.activators.size()});
						  o.set_id(utils::resources_type::object_id::gate); // TODO
						  break;
					  case activator_type::INFRARED_LASER:
						  world.interactions.push_back(
						    {model::interaction_kind::HEAVY_MIDAIR, model::interactable_kind::INFRARED_LASER, world.activators.size()});
						  o.set_id(utils::resources_type::object_id::gate); // TODO
						  break;
					  case activator_type::NONE:
						  [[fallthrough]];
					  default:
						  error("internal_error");
						  break;
				  }

				  view_handle view_handle = map_viewer_omap->add_object(std::move(o));
				  model_handle model_handle{world.activators.size(), model_handle::ACTIVATOR};
				  m_model2view[model_handle] = view_handle;
				  m_view2model[view_handle]  = model_handle;

				  m_view2name[view_handle] = std::move(activator.name);

				  world.activators.push_back({activator.target_tiles,
				                              activator.refire_after == std::numeric_limits<decltype(activator.refire_after)>::max() ?
				                                std::optional<unsigned int>{} :
				                                std::optional<unsigned int>(activator.refire_after),
				                              activator.delay, activator.activation_difficulty, activator.refire_repeat});
			  },
			  [&](const gate &gate) {
				  const float TOPLEFT_X = static_cast<float>(gate.pos.x) * model::cst::cell_width;
				  const float TOPLEFT_Y = static_cast<float>(gate.pos.y) * model::cst::cell_height;

				  world.actionables.push_back({model::actionable::instance_data{{gate.pos.x, gate.pos.y}, world.actionables.size()},
				                               model::actionable::behaviours_ns::gate});

				  view::object o{};
				  o.set_pos(TOPLEFT_X, TOPLEFT_Y);
				  o.set_id(utils::resources_type::object_id::gate);
				  view_handle view_handle = map_viewer_omap->add_object(std::move(o));
				  model_handle model_handle{world.actionables.size() - 1, model_handle::ACTIONABLE};
				  m_model2view[model_handle] = view_handle;
				  m_view2model[view_handle]  = model_handle;
				  if (gate.closed) {
					  world.map[gate.pos.x][gate.pos.y].type = model::cell_type::CHASM;
				  }
				  else {
					  map_viewer_omap->hide(view_handle);
				  }

				  m_view2name[view_handle] = gate.name;
				  m_name2view[gate.name]   = view_handle;
			  },
			  [&](const autoshooter &autoshooter) {
				  const float TOPLEFT_X = static_cast<float>(autoshooter.pos.x) * model::cst::cell_width;
				  const float TOPLEFT_Y = static_cast<float>(autoshooter.pos.y) * model::cst::cell_height;

				  world.actionables.push_back(
				    {model::actionable::instance_data{
				       {autoshooter.pos.x, autoshooter.pos.y}, world.actionables.size(), autoshooter.firing_rate, autoshooter.facing},
				     model::actionable::behaviours_ns::autoshooter});

				  view::object o{};
				  o.set_pos(TOPLEFT_X, TOPLEFT_Y);
				  o.set_id(utils::resources_type::object_id::autoshooter);
				  view_handle view_handle = map_viewer_omap->add_object(std::move(o));
				  model_handle model_handle{world.actionables.size() - 1, model_handle::ACTIONABLE};
				  m_model2view[model_handle] = view_handle;
				  m_view2model[view_handle]  = model_handle;

				  m_view2name[view_handle]      = autoshooter.name;
				  m_name2view[autoshooter.name] = view_handle;
			  }};
			std::visit(visitor, actor);
		}

		map_viewer.set_map(std::move(view_map));
	} // unlocking locks on map_viewer before moving

	state::access<adapter>::view(m_state).set_map(std::move(map_viewer));

	utils::log::info("adapter_map_loader_v1_0_0.map_loaded", "map"_a = map);
	return true;
}
