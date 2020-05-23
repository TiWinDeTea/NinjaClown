#include <bot_interface/bot.h>
#include <cpptoml.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "state_holder.hpp"
#include "utils/visitor.hpp"
#include "view/facing_dir.hpp"

// TODO ailleurs : les infobulles devraient afficher le numéro d’handle
// TODO faire des commandes pour activer des trucs via les numéros d’handle
// TODO Effets sonores

// TODO externalize error messages

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
	unsigned int delay;
	unsigned int refire_after;
	activator_type type;
	std::vector<size_t> target_tiles;
};

struct gate {
	point pos;
	bool closed;
};

struct autoshooter {
	point pos;
	unsigned int firing_rate;
	float facing;
};

namespace keys {
	constexpr const char *inherits           = "inherits";
	constexpr const char *name               = "name";
	constexpr const char *hp                 = "hp";
	constexpr const char *sprite             = "sprite";
	constexpr const char *behaviour          = "behaviour";
	constexpr const char *type               = "type";
	constexpr const char *x_pos              = "pos.x";
	constexpr const char *y_pos              = "pos.y";
	constexpr const char *facing             = "facing";
	constexpr const char *firing_rate        = "firing_rate";
	constexpr const char *closed             = "closed";
	constexpr const char *refire_after       = "refire_after";
	constexpr const char *duration           = "duration";
	constexpr const char *delay              = "delay";
	constexpr const char *acts_on_table      = "acts_on";
	constexpr const char *mobs_def_table     = "mobs.definition";
	constexpr const char *mobs_spawn_table   = "mobs.spawn";
	constexpr const char *actors_spawn_table = "actors.spawn";
	constexpr const char *map_layout         = "map.layout";
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

template <typename... Args>
void perror(std::string_view map, std::string_view fmt, Args &&... args) {
	spdlog::error("Error while parsing map \"{}\"", map);
	spdlog::error(fmt, std::forward<Args>(args)...);
}

template <typename T, typename... Args>
bool try_get_silent(const std::shared_ptr<cpptoml::table> &table, const char *key, T &value) {
	cpptoml::option<T> maybe_value = table->get_qualified_as<T>(key);
	if (maybe_value) {
		value = *maybe_value;
	}
	return maybe_value.operator bool();
}

template <typename T, typename... Args>
bool try_get(std::string_view map, const std::shared_ptr<cpptoml::table> &table, const char *key, T &value, std::string_view error_fmt,
             Args &&... error_args) {
	if (!try_get_silent(table, key, value)) {
		perror(map, error_fmt, std::forward<Args>(error_args)...);
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

[[nodiscard]] usmap<mob_definition> load_mobs_defs(const std::shared_ptr<cpptoml::table_array> &tables, std::string_view map) {
	assert(!globstr_to_behaviour.empty()); // NOLINT
	assert(!globstr_to_mob_sprite.empty()); // NOLINT

	auto error = [&map](std::string_view fmt, auto &&... vals) {
		perror(map, fmt, std::forward<decltype(vals)>(vals)...);
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
				error("Referenced mob \"{}\" was not declared", *base);
				return {};
			}
		}

		cpptoml::option<std::string> name = mob_def->get_as<std::string>(keys::name);
		if (!name) {
			error("Mob missing name (key: {})", keys::name);
			return {};
		}

		cpptoml::option<unsigned int> hp = mob_def->get_as<unsigned int>(keys::hp);
		if (hp) {
			current.hp = *hp;
		}
		else if (!base) {
			error("Mob {} has unknown hp (key: {})", *name, keys::hp);
			return {};
		}

		cpptoml::option<std::string> sprite = mob_def->get_as<std::string>(keys::sprite);
		if (auto it = globstr_to_mob_sprite.find(sprite.value_or("")); it != globstr_to_mob_sprite.end()) {
			current.sprite = it->second;
		}
		else if (!base) {
			error("Mob {} has unknown sprite (key: {})", *name, keys::sprite);
			return {};
		}

		cpptoml::option<std::string> behaviour = mob_def->get_as<std::string>(keys::behaviour);
		if (auto it = globstr_to_behaviour.find(behaviour.value_or("")); it != globstr_to_behaviour.end()) {
			current.behaviour = it->second;
		}
		else if (!base) {
			error("Mob {} has unknown behaviour (key: {})", *name, keys::behaviour);
			return {};
		}

		mobs_definitions.emplace(*name, current);
	}
	return mobs_definitions;
}

[[nodiscard]] std::vector<mob> load_mobs_spawns(const std::shared_ptr<cpptoml::table_array> &tables,
                                                const usmap<mob_definition> &definitions, std::string_view map) {
	auto error = [&map](std::string_view fmt, auto &&... vals) {
		perror(map, fmt, std::forward<decltype(vals)>(vals)...);
	};

	std::vector<mob> loaded_mobs;
	for (const auto &mob : *tables) {

		cpptoml::option<std::string> type = mob->get_as<std::string>(keys::type);
		auto parsed_type                  = definitions.find(type.value_or(""));
		if (!type || parsed_type == definitions.end()) {
			error("Encountered mob with unknown type (key: {})", keys::type);
			if (type) {
				spdlog::error("(type: {})", *type);
			}
			return {};
		}

		auto try_get = [&map, &mob, &type](const char *key, auto &value) -> bool {
			return ::try_get(map, mob, key, value, "key {} was not specified (mob type: {})", key, *type);
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
load_actors(const std::shared_ptr<cpptoml::table_array> &tables, std::string_view map) {
	assert(!globstr_to_activator_type.empty()); // NOLINT

	auto error = [&map](std::string_view fmt, auto &&... vals) {
		perror(map, fmt, std::forward<decltype(vals)>(vals)...);
	};

	std::vector<std::variant<activator, gate, autoshooter>> actors;

	std::unordered_map<std::string, size_t> actionable_handles;

	for (const auto &actor : *tables) {
		auto type = actor->get_as<std::string>(keys::type);
		if (!type) {
			error("Encountered actor with unspecified type (key: {})", keys::type);
			return {};
		}

		auto try_get = [&map, &actor, &type](const char *key, auto &value, bool silent = false) -> bool {
			if (silent) {
				return try_get_silent(actor, key, value);
			}
			else {
				return ::try_get(map, actor, key, value, "key {} was not specified (actor type: {})", key, *type);
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
				spdlog::warn("Actor {} is duplicated", name);
                spdlog::warn(R"((while parsing map "{}"))", map);
			}
			actors.emplace_back(autoshooter{point{x_pos, y_pos}, firing_rate, static_cast<float>(facing)});
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
                spdlog::warn("Actor {} is duplicated", name);
                spdlog::warn(R"((while parsing map "{}"))", map);
            }
            actors.emplace_back(gate{point{x_pos, y_pos}, closed});
		}
		else {
			activator activator;
			activator.pos = {x_pos, y_pos};

			auto parsed_type = globstr_to_activator_type.find(*type);
			if (parsed_type == globstr_to_activator_type.end()) {
				error("Unknown actor type: {} (key: {})", *type, keys::type);
				return {};
			}
			activator.type = parsed_type->second;

			activator.refire_after = std::numeric_limits<decltype(activator.refire_after)>::max();
			if (!try_get(keys::refire_after, activator.refire_after, true)) {
				try_get(keys::duration, activator.refire_after, true);
			}

			activator.delay = 0;
			try_get(keys::delay, activator.delay, true);

			auto targets = actor->get_array_of<std::string>(keys::acts_on_table);
			if (!targets) {
				error("actor {} has no target to act on (key: {})", *type, keys::acts_on_table);
				spdlog::error("(or value is malformed)", *type, keys::acts_on_table);
				return {};
			}
			for (const std::string &target : *targets) {

                auto it = actionable_handles.find(target);
				if (it == actionable_handles.end()) {
					error("Unknown referenced gate {}", target);
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

	auto error = [&map](std::string_view fmt, auto &&... vals) {
		perror(map, fmt, std::forward<decltype(vals)>(vals)...);
	};

	auto mobs_defs_toml   = map_file->get_table_array_qualified(keys::mobs_def_table);
	auto mobs_spawns_toml = map_file->get_table_array_qualified(keys::mobs_spawn_table);
	auto actors_toml      = map_file->get_table_array_qualified(keys::actors_spawn_table);
	auto map_layout_toml  = map_file->get_qualified_array_of<std::string>(keys::map_layout);

	if (!mobs_defs_toml || !mobs_spawns_toml || !actors_toml || !map_layout_toml) {
		if (!mobs_defs_toml) {
			error("Missing mobs definitions");
		}
		if (!mobs_spawns_toml) {
			error("Missing mobs placings");
		}
		if (!actors_toml) {
			error("Missing actors");
		}
		if (!map_layout_toml) {
			error("Missing map layout");
		}
		return false;
	}

	usmap<mob_definition> mobs_defs = load_mobs_defs(mobs_defs_toml, map);
	if (mobs_defs.empty()) {
		error("Error while loading mobs definitions");
		return false;
	}

	std::vector<mob> mobs = load_mobs_spawns(mobs_spawns_toml, mobs_defs, map);
	if (mobs.empty()) {
		error("Error while loading mob spawn locations");
		return false;
	}
	if (mobs.size() > model::cst::max_entities) {
		error("Too many mob spawns");
		return false;
	}

	std::vector<std::variant<activator, gate, autoshooter>> actors;
	{
		auto maybe_actors = load_actors(actors_toml, map);
		if (!maybe_actors) {
			return false;
		}
		actors = std::move(*maybe_actors);
	}

	size_t map_height = map_layout_toml->size();
	if (map_height == 0) {
		error("The map is empty");
		return false;
	}
	size_t map_width = map_layout_toml->begin()->size();

	view::viewer &view  = state::access<adapter>::view(m_state);
	model::world &world = state::access<adapter>::model(m_state).world;


	std::vector<std::vector<view::map::cell>> view_map{map_width, {map_height, {view::map::cell::abyss}}};
	world.grid.resize(map_width, map_height);

	unsigned int line_idx{0};
	for (const std::string &line : *map_layout_toml) {

		if (line.size() != map_width) {
			error("Non-coherent map dimensions (at line {})", line_idx + 1);
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
				case 'T':
					world.grid[column_idx][line_idx].type = model::cell_type::TARGET;
					view_map[column_idx][line_idx]        = view::map::cell::target_tile;
					break;
				default:
					error("Unknown caracter '{}' in map layout", line[column_idx]);
					return false;
			}
		}

		++line_idx;
	}


	// TODO externalize
	const float DEFAULT_HITBOX_HALF_WIDTH  = 0.5f;
	const float DEFAULT_HITBOX_HALF_HEIGHT = 0.5f;

	size_t model_entity_handle = 0;
	for (const mob &mob : mobs) {
		const float CENTER_X = static_cast<float>(mob.pos.x) * model::cell_width + model::cell_width / 2.f;
		const float CENTER_Y = static_cast<float>(mob.pos.y) * model::cell_height + model::cell_height / 2.f;

		switch (mob.type.behaviour) {
			case mob_behaviour::NONE:
				world.components.metadata[model_entity_handle].kind = bot::entity_kind::EK_HARMLESS;
				break;
			case mob_behaviour::SCIENTIST:
				break;
			case mob_behaviour::CLOWN:
				// TODO
				break;
			case mob_behaviour::DLL:
				world.components.metadata[model_entity_handle].kind = bot::entity_kind::EK_DLL;
				world.ninja_clown_handle                            = model_entity_handle; // TODO : multi-handle for DLL
				break;
		}

		utils::resource_manager::mob_id resource_id;
		switch (mob.type.sprite) {
			case mob_sprite::NONE:
				error("Internal error");
				return false;
			case mob_sprite::SCIENTIST:
				resource_id = utils::resource_manager::mob_id::scientist;
				break;
			case mob_sprite::CLOWN:
				resource_id = utils::resource_manager::mob_id::player;
				break;
		}

		world.components.health[model_entity_handle] = {static_cast<std::uint8_t>(mob.type.hp)};
		world.components.hitbox[model_entity_handle] = {CENTER_X, CENTER_Y, DEFAULT_HITBOX_HALF_WIDTH, DEFAULT_HITBOX_HALF_HEIGHT};

		model::component::hitbox &hitbox = *world.components.hitbox[model_entity_handle];

		view::mob m{};
		m.set_mob_id(resource_id, m_state.resources);
		m.set_direction(view::facing_direction::from_angle(mob.facing));
		m.set_pos(hitbox.center.x, hitbox.center.y);

		view_handle view_handle = view.acquire_overmap()->add_mob(std::move(m));
		model_handle model_handle{model_entity_handle};
		m_mobs_model2view[model_handle] = view_handle;
		m_view2model[view_handle]  = model_handle;

		++model_entity_handle;
	}

	for (const std::variant<activator, gate, autoshooter> &actor : actors) {

		utils::visitor visitor{
		  [&](const activator &activator) {
			  const float TOPLEFT_X = static_cast<float>(activator.pos.x) * model::cell_width;
			  const float TOPLEFT_Y = static_cast<float>(activator.pos.y) * model::cell_height;

			  world.grid[activator.pos.x][activator.pos.y].interaction_handle = {world.interactions.size()};

			  view::object o{};
			  o.set_pos(TOPLEFT_X, TOPLEFT_Y);

			  switch (activator.type) {

				  case activator_type::BUTTON:
					  world.interactions.push_back(
					    {model::interaction_kind::LIGHT_MANUAL, model::interactable_kind::BUTTON, world.activators.size()});
                      o.set_id(utils::resource_manager::object_id::button, m_state.resources);
					  break;
				  case activator_type::INDUCTION_LOOP:
					  world.interactions.push_back(
					    {model::interaction_kind::LIGHT_MIDAIR, model::interactable_kind::INDUCTION_LOOP, world.activators.size()});
                      o.set_id(utils::resource_manager::object_id::gate, m_state.resources); // TODO
					  break;
				  case activator_type::INFRARED_LASER:
					  world.interactions.push_back(
					    {model::interaction_kind::HEAVY_MIDAIR, model::interactable_kind::INFRARED_LASER, world.activators.size()});
                      o.set_id(utils::resource_manager::object_id::gate, m_state.resources); // TODO
					  break;
				  case activator_type::NONE:
					  [[fallthrough]];
				  default:
					  error("Internal error");
					  break;
			  }

              view_handle view_handle = view.acquire_overmap()->add_object(std::move(o));
              model_handle model_handle{world.activators.size()};
              // m_model2view[model_handle] = view_handle; // fixme: clashes with mob handles. Model2View for activators can be ignored for now AFAIK
              m_view2model[view_handle]  = model_handle;

              world.activators.push_back({std::move(activator.target_tiles),
                                          activator.refire_after == std::numeric_limits<decltype(activator.refire_after)>::max() ?
                                          std::optional<unsigned int>{} :
                                          std::optional<unsigned int>(activator.refire_after),
                                          activator.delay});
		  },
		  [&](const gate &gate) {
			  const float TOPLEFT_X = static_cast<float>(gate.pos.x) * model::cell_width;
              const float TOPLEFT_Y = static_cast<float>(gate.pos.y) * model::cell_height;

              world.actionables.push_back({model::actionable::instance_data{{gate.pos.x, gate.pos.y}}, model::actionable::behaviours_ns::gate});

              view::object o{};
              o.set_pos(TOPLEFT_X, TOPLEFT_Y);
			  o.set_id(utils::resource_manager::object_id::gate, m_state.resources);
              view_handle view_handle = view.acquire_overmap()->add_object(std::move(o));
              model_handle model_handle{world.actionables.size() - 1};
              m_gates_model2view[model_handle] = view_handle;
              m_view2model[view_handle]  = model_handle;
			  if (gate.closed) {
				  world.grid[gate.pos.x][gate.pos.y].type = model::cell_type::CHASM;
			  } else {
                  view.acquire_overmap()->hide(view_handle);
			  }
		  },
		  [&](const autoshooter &autoshooter) {
			  // TODO
		  }};
		std::visit(visitor, actor);
	}

	view.set_map(std::move(view_map));

	spdlog::info("Loaded map \"{}\"", map);
	return true;
}
