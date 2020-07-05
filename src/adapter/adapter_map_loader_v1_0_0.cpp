#include <cpptoml.h>
#include <ninja_clown/api.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"
#include "utils/visitor.hpp"
#include "view/facing_dir.hpp"
#include "view/map.hpp"
#include "view/mob.hpp"
#include "view/object.hpp"
#include "view/viewer.hpp"

using fmt::literals::operator""_a;

namespace {

template <typename T>
using usmap = std::unordered_map<std::string, T>;

struct point {
	int x, y;
};

enum class mob_behaviour {
	NONE,
	SCIENTIST,
	CLOWN,
	DLL,
};

enum class mob_sprite {
	NONE,
	SCIENTIST,
	CLOWN,
};

struct mob_definition {
	unsigned int hp;
	model::tick_t attack_delay;
	model::tick_t throw_delay;
	mob_behaviour behaviour;
	mob_sprite sprite;
};

struct mob {
	point pos;
	float facing;
	const mob_definition &type;
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
	model::tick_t activation_difficulty;
	activator_type type;
	std::vector<size_t> target_tiles;
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

namespace keys {
	constexpr const char *inherits              = "inherits";
	constexpr const char *name                  = "name";
	constexpr const char *hp                    = "hp";
	constexpr const char *attack_delay          = "attack_delay";
	constexpr const char *throw_delay           = "throw_delay";
	constexpr const char *sprite                = "sprite";
	constexpr const char *behaviour             = "behaviour";
	constexpr const char *type                  = "type";
	constexpr const char *x_pos                 = "pos.x";
	constexpr const char *y_pos                 = "pos.y";
	constexpr const char *facing                = "facing";
	constexpr const char *firing_rate           = "firing_rate";
	constexpr const char *closed                = "closed";
	constexpr const char *refire_after          = "refire_after";
	constexpr const char *activation_difficulty = "activation_difficulty";
	constexpr const char *duration              = "duration";
	constexpr const char *delay                 = "delay";
	constexpr const char *acts_on_table         = "acts_on";
	constexpr const char *mobs_def_table        = "mobs.definition";
	constexpr const char *mobs_spawn_table      = "mobs.spawn";
	constexpr const char *actors_spawn_table    = "actors.spawn";
	constexpr const char *map_layout            = "map.layout";
} // namespace keys

namespace values {
	constexpr const char *none           = "none";
	constexpr const char *scientist      = "scientist";
	constexpr const char *dll            = "dll";
	constexpr const char *clown          = "clown";
	constexpr const char *button         = "button";
	constexpr const char *induction_loop = "induction_loop";
	constexpr const char *infrared_laser = "infrared_laser";
	constexpr const char *autoshooter    = "autoshooter";
	constexpr const char *gate           = "gate";
} // namespace values

usmap<mob_behaviour> globstr_to_behaviour;
usmap<mob_sprite> globstr_to_mob_sprite;
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
bool try_get(const std::shared_ptr<cpptoml::table> &table, const char *key, T &value, std::string_view fmt, Args &&... error_args) {
	if (!try_get_silent(table, key, value)) {
		spdlog::error(fmt, std::forward<Args>(error_args)...);
		return false;
	}
	return true;
}

void init_maps() {
	if (globstr_to_activator_type.empty()) {
		globstr_to_behaviour.emplace(values::none, mob_behaviour::NONE);
		globstr_to_behaviour.emplace(values::scientist, mob_behaviour::SCIENTIST);
		globstr_to_behaviour.emplace(values::dll, mob_behaviour::DLL);
		globstr_to_behaviour.emplace(values::clown, mob_behaviour::CLOWN);
		globstr_to_mob_sprite.emplace(values::scientist, mob_sprite::SCIENTIST);
		globstr_to_mob_sprite.emplace(values::clown, mob_sprite::CLOWN);
		globstr_to_activator_type.emplace(values::button, activator_type::BUTTON);
		globstr_to_activator_type.emplace(values::induction_loop, activator_type::INDUCTION_LOOP);
		globstr_to_activator_type.emplace(values::infrared_laser, activator_type::INFRARED_LASER);
	}
}

[[nodiscard]] usmap<mob_definition> load_mobs_defs(const utils::resource_manager &res, const std::shared_ptr<cpptoml::table_array> &tables,
                                                   std::string_view map) {
	assert(!globstr_to_behaviour.empty()); // NOLINT
	assert(!globstr_to_mob_sprite.empty()); // NOLINT

	auto error = [&map, &res](const char *key, auto &&... vals) {
		utils::log::error(res, std::string("adapter_map_loader_v1_0_0.") + key, std::forward<decltype(vals)>(vals)..., "map"_a = map);
	};

	usmap<mob_definition> mobs_definitions;
	for (const auto &mob_def : *tables) {
		mob_definition current{};
		cpptoml::option<std::string> base = mob_def->get_as<std::string>(keys::inherits);
		if (base) {
			if (auto it = mobs_definitions.find(*base); it != mobs_definitions.end()) {
				current = it->second;
			}
			else {
				error("unknown_mob_reference", "mob"_a = *base);
				return {};
			}
		}

		cpptoml::option<std::string> name = mob_def->get_as<std::string>(keys::name);
		if (!name) {
			error("mob_missing_name", "key"_a = keys::name);
			return {};
		}

		cpptoml::option<unsigned int> hp = mob_def->get_as<unsigned int>(keys::hp);
		if (hp) {
			current.hp = *hp;
		}
		else if (!base) {
			error("mob_missing_hp", "name"_a = *name, "key"_a = keys::hp);
			return {};
		}

		current.throw_delay = model::component::default_throw_delay;
		try_get_silent(mob_def, keys::throw_delay, current.throw_delay);

		current.attack_delay = model::component::default_attack_delay;
		try_get_silent(mob_def, keys::attack_delay, current.attack_delay);

		cpptoml::option<std::string> sprite = mob_def->get_as<std::string>(keys::sprite);
		if (auto it = globstr_to_mob_sprite.find(sprite.value_or("")); it != globstr_to_mob_sprite.end()) {
			current.sprite = it->second;
		}
		else if (!base) {
			error("mob_missing_sprite", "name"_a = *name, "key"_a = keys::sprite);
			return {};
		}

		cpptoml::option<std::string> behaviour = mob_def->get_as<std::string>(keys::behaviour);
		if (auto it = globstr_to_behaviour.find(behaviour.value_or("")); it != globstr_to_behaviour.end()) {
			current.behaviour = it->second;
		}
		else if (!base) {
			error("mob_missing_behaviour", *name, keys::behaviour);
			return {};
		}

		mobs_definitions.emplace(*name, current);
	}
	return mobs_definitions;
}

[[nodiscard]] std::vector<mob> load_mobs_spawns(const utils::resource_manager &res, const std::shared_ptr<cpptoml::table_array> &tables,
                                                const usmap<mob_definition> &definitions, std::string_view map) {
	std::vector<mob> loaded_mobs;
	for (const auto &mob : *tables) {

		cpptoml::option<std::string> type = mob->get_as<std::string>(keys::type);
		auto parsed_type                  = definitions.find(type.value_or(""));
		if (!type || parsed_type == definitions.end()) {
			utils::log::error(res, "adapter_map_loader_v1_0_0.mob_with_unknown_type", "map"_a = map, "key"_a = keys::type);
			if (type) {
				utils::log::error(res, "adapter_map_loader_v1_0_0.mob_type_not_recognized", "type"_a = *type);
			}
			return {};
		}

		auto try_get = [&res, &map, &mob, &type](const char *key, auto &value) -> bool {
			return ::try_get(mob, key, value, utils::log::get_or_gen(res, "adapter_map_loader_v1_0_0.mob_spawn_missing_component"),
			                 "map"_a = map, "type"_a = *type, "key"_a = key);
		};

		int x_pos{};
		if (!try_get(keys::x_pos, x_pos)) {
			return {};
		}

		int y_pos{};
		if (!try_get(keys::y_pos, y_pos)) {
			return {};
		}

		double facing{};
		if (!try_get(keys::facing, facing)) {
			return {};
		}

		loaded_mobs.emplace_back(::mob{point{x_pos, y_pos}, static_cast<float>(facing), parsed_type->second});
	}
	return loaded_mobs;
}

[[nodiscard]] std::optional<std::vector<std::variant<activator, gate, autoshooter>>>
load_actors(const utils::resource_manager &res, const std::shared_ptr<cpptoml::table_array> &tables, std::string_view map) {
	assert(!globstr_to_activator_type.empty()); // NOLINT

	auto error = [&map, &res](const char *key, auto &&... vals) {
		utils::log::error(res, std::string("adapter_map_loader_v1_0_0.") + key, "map"_a = map, std::forward<decltype(vals)>(vals)...);
	};

	std::vector<std::variant<activator, gate, autoshooter>> actors;

	std::unordered_map<std::string, size_t> actionable_handles;

	for (const auto &actor : *tables) {
		auto type = actor->get_as<std::string>(keys::type);
		if (!type) {
			error("actor_with_unknown_type", "key"_a = keys::type);
			return {};
		}

		auto try_get = [&map, &res, &actor, &type](const char *key, auto &value, bool silent = false) -> bool {
			if (silent) {
				return try_get_silent(actor, key, value);
			}
			else {
				return ::try_get(actor, key, value, utils::log::get_or_gen(res, "adapter_map_loader_v1_0_0.actor_missing_component"),
				                 "key"_a = key, "type"_a = *type);
			}
		};

		int x_pos{};
		if (!try_get(keys::x_pos, x_pos)) {
			return {};
		}

		int y_pos{};
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
				utils::log::warn(res, "adapter_map_loader_v1_0_0.actor_duplication", "actor"_a = name, "map"_a = map);
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
				utils::log::warn(res, "adapter_map_loader_v1_0_0.actor_duplication", "actor"_a = name, "map"_a = map);
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

			activator.refire_after = std::numeric_limits<decltype(activator.refire_after)>::max();
			if (!try_get(keys::refire_after, activator.refire_after, true)) {
				try_get(keys::duration, activator.refire_after, true);
			}

			activator.activation_difficulty = model::default_activation_difficulty;
			try_get(keys::activation_difficulty, activator.activation_difficulty, true);

			activator.delay = model::default_activation_delay;
			try_get(keys::delay, activator.delay, true);

			auto targets = actor->get_array_of<std::string>(keys::acts_on_table);
			if (!targets) {
				error("actor_with_no_target", "type"_a = *type, keys::acts_on_table);
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

void test(const class maclasse &tes) { }

} // namespace

bool adapter::adapter::load_map_v1_0_0(const std::shared_ptr<cpptoml::table> &map_file, std::string_view map) noexcept {
	init_maps();

	auto error = [&map, this](const char *key, auto &&... vals) {
		utils::log::error(m_state.resources(), std::string("adapter_map_loader_v1_0_0.") + key, "map"_a = map,
		                  std::forward<decltype(vals)>(vals)...);
	};

	auto mobs_defs_toml   = map_file->get_table_array_qualified(keys::mobs_def_table);
	auto mobs_spawns_toml = map_file->get_table_array_qualified(keys::mobs_spawn_table);
	auto actors_toml      = map_file->get_table_array_qualified(keys::actors_spawn_table);
	auto map_layout_toml  = map_file->get_qualified_array_of<std::string>(keys::map_layout);

	if (!mobs_defs_toml || !mobs_spawns_toml || !actors_toml || !map_layout_toml) {
		if (!mobs_defs_toml) {
			error("missing_mob_def");
		}
		if (!mobs_spawns_toml) {
			error("missing_mob_placings");
		}
		if (!actors_toml) {
			error("missing_actors");
		}
		if (!map_layout_toml) {
			error("missing_map_layout");
		}
		return false;
	}

	usmap<mob_definition> mobs_defs = load_mobs_defs(m_state.resources(), mobs_defs_toml, map);
	if (mobs_defs.empty()) {
		error("bad_mob_defs", "map"_a = map);
		return false;
	}

	std::vector<mob> mobs = load_mobs_spawns(m_state.resources(), mobs_spawns_toml, mobs_defs, map);
	if (mobs.empty()) {
		error("bad_mob_spawns", "map"_a = map);
		return false;
	}
	if (mobs.size() > model::cst::max_entities) {
		error("too_many_spawns", "map"_a = map);
		return false;
	}

	std::vector<std::variant<activator, gate, autoshooter>> actors;
	{
		auto maybe_actors = load_actors(m_state.resources(), actors_toml, map);
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

	view::viewer &view  = state::access<adapter>::view(m_state);
	model::world &world = state::access<adapter>::model(m_state).world;

	std::vector<std::vector<view::map::cell>> view_map{map_width, {map_height, {view::map::cell::abyss}}};
	world.grid.resize(map_width, map_height);

	unsigned int line_idx{0};
	for (const std::string &line : *map_layout_toml) {

		if (line.size() != map_width) {
			utils::log::error(m_state.resources(), "adapter_map_loader_v1_0_0.bad_map_size", "map"_a = map, "line"_a = line_idx + 1);
			return false;
		}

		for (size_t column_idx = 0; column_idx < map_width; ++column_idx) {
			switch (line[column_idx]) {
				case '#':
					world.grid[column_idx][line_idx].type = model::cell_type::CHASM;
					view_map[column_idx][line_idx]        = view::map::cell::abyss;
					break;
				case ' ':
					world.grid[column_idx][line_idx].type = model::cell_type::GROUND;
					view_map[column_idx][line_idx]        = view::map::cell::concrete_tile;
					break;
				case '~':
					world.grid[column_idx][line_idx].type = model::cell_type::GROUND;
					view_map[column_idx][line_idx]        = view::map::cell::iron_tile;
					break;
				case 'T': {
					world.grid[column_idx][line_idx].type = model::cell_type::GROUND;
					view_map[column_idx][line_idx]        = view::map::cell::concrete_tile;
					world.target_tile = {static_cast<utils::ssize_t>(column_idx), static_cast<utils::ssize_t>(line_idx)};

					view::object obj;
					obj.set_id(utils::resources_type::object_id::target, m_state.resources());
					obj.set_pos(world.target_tile.x * model::cst::cell_width, world.target_tile.y * model::cst::cell_height);
					obj.reveal();
					m_target_handle = view.acquire_overmap()->add_object(std::move(obj));
					break;
				}
				default:
					error("bad_map_char", "char"_a = line[column_idx], "line"_a = line_idx + 1, "column"_a = column_idx + 1);
					return false;
			}
		}

		++line_idx;
	}

	// TODO externalize
	const float DEFAULT_HITBOX_HALF_WIDTH  = 0.25f;
	const float DEFAULT_HITBOX_HALF_HEIGHT = 0.25f;

	size_t model_entity_handle = 0;
	for (const mob &mob : mobs) {
		const float CENTER_X = static_cast<float>(mob.pos.x) * model::cst::cell_width + model::cst::cell_width / 2.f;
		const float CENTER_Y = static_cast<float>(mob.pos.y) * model::cst::cell_height + model::cst::cell_height / 2.f;

		switch (mob.type.behaviour) {
			case mob_behaviour::NONE:
				world.components.metadata[model_entity_handle].kind = ninja_api::nnj_entity_kind::EK_HARMLESS;
				break;
			case mob_behaviour::SCIENTIST:
				// TODO
				break;
			case mob_behaviour::CLOWN:
				// TODO
				break;
			case mob_behaviour::DLL:
				world.components.metadata[model_entity_handle].kind = ninja_api::nnj_entity_kind::EK_DLL;
				break;
		}

		utils::resources_type::mob_id resource_id;
		switch (mob.type.sprite) {
			case mob_sprite::NONE:
				error("Internal error");
				return false;
			case mob_sprite::SCIENTIST:
				resource_id = utils::resources_type::mob_id::scientist;
				break;
			case mob_sprite::CLOWN:
				resource_id = utils::resources_type::mob_id::player;
				break;
		}

		world.components.health[model_entity_handle] = {static_cast<std::uint8_t>(mob.type.hp)};
		world.components.hitbox[model_entity_handle] = {CENTER_X, CENTER_Y, DEFAULT_HITBOX_HALF_WIDTH, DEFAULT_HITBOX_HALF_HEIGHT};
		world.components.properties[model_entity_handle].throw_delay  = mob.type.throw_delay;
		world.components.properties[model_entity_handle].attack_delay = mob.type.attack_delay;

		model::component::hitbox &hitbox = *world.components.hitbox[model_entity_handle];

		view::mob m{};
		m.set_mob_id(resource_id, m_state.resources());
		m.set_direction(view::facing_direction::from_angle(mob.facing));
		m.set_pos(hitbox.center.x, hitbox.center.y);

		view_handle view_handle = view.acquire_overmap()->add_mob(std::move(m));
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

			  world.grid[activator.pos.x][activator.pos.y].interaction_handle = {world.interactions.size()};

			  view::object o{};
			  o.set_pos(TOPLEFT_X, TOPLEFT_Y);

			  switch (activator.type) {

				  case activator_type::BUTTON:
					  world.interactions.push_back(
					    {model::interaction_kind::LIGHT_MANUAL, model::interactable_kind::BUTTON, world.activators.size()});
					  o.set_id(utils::resources_type::object_id::button, m_state.resources());
					  break;
				  case activator_type::INDUCTION_LOOP:
					  world.interactions.push_back(
					    {model::interaction_kind::LIGHT_MIDAIR, model::interactable_kind::INDUCTION_LOOP, world.activators.size()});
					  o.set_id(utils::resources_type::object_id::gate, m_state.resources()); // TODO
					  break;
				  case activator_type::INFRARED_LASER:
					  world.interactions.push_back(
					    {model::interaction_kind::HEAVY_MIDAIR, model::interactable_kind::INFRARED_LASER, world.activators.size()});
					  o.set_id(utils::resources_type::object_id::gate, m_state.resources()); // TODO
					  break;
				  case activator_type::NONE:
					  [[fallthrough]];
				  default:
					  error("internal_error");
					  break;
			  }

			  view_handle view_handle = view.acquire_overmap()->add_object(std::move(o));
			  model_handle model_handle{world.activators.size(), model_handle::ACTIVATOR};
			  m_model2view[model_handle] = view_handle;
			  m_view2model[view_handle]  = model_handle;

			  world.activators.push_back({activator.target_tiles,
			                              activator.refire_after == std::numeric_limits<decltype(activator.refire_after)>::max() ?
			                                std::optional<unsigned int>{} :
			                                std::optional<unsigned int>(activator.refire_after),
			                              activator.delay, activator.activation_difficulty});
		  },
		  [&](const gate &gate) {
			  const float TOPLEFT_X = static_cast<float>(gate.pos.x) * model::cst::cell_width;
			  const float TOPLEFT_Y = static_cast<float>(gate.pos.y) * model::cst::cell_height;

			  world.actionables.push_back({model::actionable::instance_data{{gate.pos.x, gate.pos.y}, world.actionables.size()},
			                               model::actionable::behaviours_ns::gate});

			  view::object o{};
			  o.set_pos(TOPLEFT_X, TOPLEFT_Y);
			  o.set_id(utils::resources_type::object_id::gate, m_state.resources());
			  view_handle view_handle = view.acquire_overmap()->add_object(std::move(o));
			  model_handle model_handle{world.actionables.size() - 1, model_handle::ACTIONABLE};
			  m_model2view[model_handle] = view_handle;
			  m_view2model[view_handle]  = model_handle;
			  if (gate.closed) {
				  world.grid[gate.pos.x][gate.pos.y].type = model::cell_type::CHASM;
			  }
			  else {
				  view.acquire_overmap()->hide(view_handle);
			  }

			  m_view2name[view_handle] = gate.name;
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
			  o.set_id(utils::resources_type::object_id::autoshooter, m_state.resources());
			  view_handle view_handle = view.acquire_overmap()->add_object(std::move(o));
			  model_handle model_handle{world.actionables.size() - 1, model_handle::ACTIONABLE};
			  m_model2view[model_handle] = view_handle;
			  m_view2model[view_handle]  = model_handle;

			  m_view2name[view_handle] = autoshooter.name;
		  }};
		std::visit(visitor, actor);
	}

	view.set_map(std::move(view_map));

	utils::log::info(m_state.resources(), "adapter_map_loader_v1_0_0.map_loaded", "map"_a = map);
	return true;
}
