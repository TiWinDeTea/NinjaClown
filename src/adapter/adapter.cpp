#include <bot_interface/bot.h>
#include <cpptoml.h>
#include <fstream>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "state_holder.hpp"
#include "utils/visitor.hpp"
#include "view/facing_dir.hpp"

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
	std::vector<point> target_tiles;
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

usmap<mob_behaviour> globstr_to_behaviour;
usmap<mob_sprite> globstr_to_mob_sprite;
usmap<activator_type> globstr_to_activator_type;

void init_maps() {
	if (globstr_to_activator_type.empty()) {
		globstr_to_behaviour.emplace("none", mob_behaviour::NONE);
		globstr_to_behaviour.emplace("scientist", mob_behaviour::SCIENTIST);
		globstr_to_behaviour.emplace("dll", mob_behaviour::DLL);
		globstr_to_behaviour.emplace("clown", mob_behaviour::CLOWN);
		globstr_to_mob_sprite.emplace("scientist", mob_sprite::SCIENTIST);
		globstr_to_mob_sprite.emplace("clown", mob_sprite::CLOWN);
		globstr_to_activator_type.emplace("button", activator_type::BUTTON);
		globstr_to_activator_type.emplace("induction_loop", activator_type::INDUCTION_LOOP);
		globstr_to_activator_type.emplace("infrared_laser", activator_type::INFRARED_LASER);
	}
}

// TODO externalize error messages
// TODO clean-up missed-duplicates
// TODO externalize magic strings
[[nodiscard]] usmap<mob_definition> load_mobs_defs(const std::shared_ptr<cpptoml::table_array> &tables, std::string_view map) {
	assert(!globstr_to_behaviour.empty()); // NOLINT
	assert(!globstr_to_mob_sprite.empty()); // NOLINT

	usmap<mob_definition> mobs_definitions;
	for (const auto &mob_def : *tables) {
		mob_definition current{};
		cpptoml::option<std::string> base = mob_def->get_as<std::string>("inherits");
		if (base) {
			if (auto it = mobs_definitions.find(*base); it != mobs_definitions.end()) {
				current = it->second;
			}
			else {
				spdlog::error("Error while parsing map \"{}\"", map);
				spdlog::error("Referenced mob \"{}\" was not declared", *base);
				return {};
			}
		}

		cpptoml::option<std::string> name = mob_def->get_as<std::string>("name");
		if (!name) {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("Mob missing name");
			return {};
		}

		cpptoml::option<unsigned int> hp = mob_def->get_as<unsigned int>("hp");
		if (hp) {
			current.hp = *hp;
		}
		else if (!base) {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("Mob {} has unknown hp", *name);
			return {};
		}

		cpptoml::option<std::string> sprite = mob_def->get_as<std::string>("sprite");
		if (auto it = globstr_to_mob_sprite.find(sprite.value_or("")); it != globstr_to_mob_sprite.end()) {
			current.sprite = it->second;
		}
		else {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("Mob {} has unknown sprite", *name);
			return {};
		}

		cpptoml::option<std::string> behaviour = mob_def->get_as<std::string>("behaviour");
		if (auto it = globstr_to_behaviour.find(behaviour.value_or("")); it != globstr_to_behaviour.end()) {
			current.behaviour = it->second;
		}
		else {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("Mob {} has unknown behaviour", *name);
			return {};
		}

		mobs_definitions.emplace(*name, current);
	}
	return mobs_definitions;
}

[[nodiscard]] std::vector<mob> load_mobs_spawns(const std::shared_ptr<cpptoml::table_array> &tables,
                                                const usmap<mob_definition> &definitions, std::string_view map) {
	std::vector<mob> loaded_mobs;
	for (const auto &mob : *tables) {

		cpptoml::option<std::string> type = mob->get_as<std::string>("type");
		auto parsed_type                  = definitions.find(type.value_or(""));
		if (!type || parsed_type == definitions.end()) {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("Encountered mob with unknown type");
			if (type) {
				spdlog::error("(type: {})", *type);
			}
			return {};
		}

		cpptoml::option<int> x_pos = mob->get_qualified_as<int>("pos.x");
		if (!x_pos) {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("x_pos was not specified");
			spdlog::error("(mob type: {})", *type);
			return {};
		}

		cpptoml::option<int> y_pos = mob->get_qualified_as<int>("pos.y");
		if (!y_pos) {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("y_pos was not specified");
			spdlog::error("(mob type: {})", *type);
			return {};
		}

		cpptoml::option<double> facing = mob->get_as<double>("facing");
		if (!facing) {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("facing was not specified");
			spdlog::error("(mob type: {})", *type);
			return {};
		}

		loaded_mobs.emplace_back(::mob{point{*x_pos, *y_pos}, static_cast<float>(*facing), parsed_type->second});
	}
	return loaded_mobs;
}

[[nodiscard]] std::optional<std::vector<std::variant<activator, gate, autoshooter>>>
load_actors(const std::shared_ptr<cpptoml::table_array> &tables, std::string_view map) {
	assert(!globstr_to_activator_type.empty()); // NOLINT

	std::vector<std::variant<activator, gate, autoshooter>> actors;

	for (const auto &actor : *tables) {
		auto type = actor->get_as<std::string>("type");
		if (!type) {
			spdlog::error("Error while parsing map \"{}\"");
			spdlog::error("Encountered actor with unspecified type");
			return {};
		}

		cpptoml::option<int> x_pos = actor->get_qualified_as<int>("pos.x");
		if (!x_pos) {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("x_pos was not specified");
			spdlog::error("(actor type: {})", *type);
			return {};
		}

		cpptoml::option<int> y_pos = actor->get_qualified_as<int>("pos.y");
		if (!y_pos) {
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("y_pos was not specified");
			spdlog::error("(actor type: {})", *type);
			return {};
		}

		if (*type == "autoshooter") {
			cpptoml::option<unsigned int> firing_rate = actor->get_as<unsigned int>("firing_rate");
			if (!firing_rate) {
				spdlog::error("Error while parsing map \"{}\"", map);
				spdlog::error("firing_rate was not specified");
				spdlog::error("(actor type: {})", *type);
				return {};
			}

			cpptoml::option<double> facing = actor->get_as<double>("facing");
			if (!facing) {
				spdlog::error("Error while parsing map \"{}\"", map);
				spdlog::error("facing was not specified");
				spdlog::error("(actor type: {})", *type);
				return {};
			}

			actors.emplace_back(autoshooter{point{*x_pos, *y_pos}, *firing_rate, static_cast<float>(*facing)});
		}
		else if (*type == "gate") {

			cpptoml::option<bool> closed = actor->get_as<bool>("closed");
			if (!closed) {
				spdlog::error("Error while parsing map \"{}\"", map);
				spdlog::error("facing was not specified");
				spdlog::error("(actor type: {})", *type);
				return {};
			}

			actors.emplace_back(gate{point{*x_pos, *y_pos}, *closed});
		}
		else {
			activator activator;
			activator.pos = {*x_pos, *y_pos};

			auto parsed_type = globstr_to_activator_type.find(*type);
			if (parsed_type == globstr_to_activator_type.end()) {
				spdlog::error("Error while parsing map \"{}\"", map);
				spdlog::error("Unknown actor type: {}", *type);
				return {};
			}
			activator.type = parsed_type->second;

			cpptoml::option<unsigned int> refire_after_opt = actor->get_as<unsigned int>("refire_after");
			cpptoml::option<unsigned int> duration_opt     = actor->get_as<unsigned int>("duration");

			activator.refire_after = refire_after_opt.value_or(duration_opt.value_or(std::numeric_limits<unsigned int>::max()));
			activator.delay        = actor->get_as<unsigned int>("delay").value_or(0);

			auto targets = actor->get_array("acts_on");
			if (!targets) {
				spdlog::error("Error while parsing map \"{}\"", map);
				spdlog::error("actor {} has no target to act on", *type);
				return {};
			}
			for (const auto &target : *targets) {
				auto sub_array = target->as_array();
				if (!sub_array) {
					spdlog::error("Error while parsing map \"{}\"", map);
					spdlog::error("Malformed target list for actor {}", *type);
					return {};
				}

				std::vector<std::shared_ptr<cpptoml::value<std::int64_t>>> target_xy = sub_array->array_of<int64_t>();
				if (target_xy.size() != 2 || !target_xy.front() || !target_xy.back()) {
					spdlog::error("Error while parsing map \"{}\"", map);
					spdlog::error("Malformed target list for actor {}", *type);
					return {};
				}

				activator.target_tiles.push_back({static_cast<int>(target_xy.front()->get()), static_cast<int>(target_xy.back()->get())});
			}
			actors.emplace_back(std::move(activator));
		}
	}

	return actors;
}

} // namespace

// TODO externalize error messages
// TODO clean-up missed-duplicates
// TODO externalize magic strings
bool adapter::adapter::load_map_v1_0_0(const std::shared_ptr<cpptoml::table> &map_file, std::string_view map) noexcept {

	auto mobs_defs_toml   = map_file->get_table_array_qualified("mobs.definition");
	auto mobs_spawns_toml = map_file->get_table_array_qualified("mobs.spawn");
	auto actors_toml      = map_file->get_table_array_qualified("actors.spawn");
	auto map_layout_toml  = map_file->get_qualified_array_of<std::string>("map.layout");

	if (!mobs_defs_toml || !mobs_spawns_toml || !actors_toml || !map_layout_toml) {
		spdlog::error("Error while parsing map \"{}\"", map);
		if (!mobs_defs_toml) {
			spdlog::error("Missing mobs definitions");
		}
		if (!mobs_spawns_toml) {
			spdlog::error("Missing mobs placings");
		}
		if (!actors_toml) {
			spdlog::error("Missing actors");
		}
		if (!map_layout_toml) {
			spdlog::error("Missing map layout");
		}
		return false;
	}

	usmap<mob_definition> mobs_defs = load_mobs_defs(mobs_defs_toml, map);
	if (mobs_defs.empty()) {
		spdlog::error("Error while parsing map \"{}\"", map);
		spdlog::error("Error while loading mobs definitions");
		return false;
	}

	std::vector<mob> mobs = load_mobs_spawns(mobs_spawns_toml, mobs_defs, map);
	if (mobs.empty()) {
		spdlog::error("Error while parsing map \"{}\"", map);
		spdlog::error("Error while loading mob spawn locations");
		return false;
	}
	if (mobs.size() > model::max_entities) {
		spdlog::error("Error while loading map \"{}\"", map);
		spdlog::error("Too many mob spawns");
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
		spdlog::error("Error while parsing map \"{}\"", map);
		spdlog::error("The map is empty");
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
			spdlog::error("Error while parsing map \"{}\"", map);
			spdlog::error("Non-coherent map dimensions");
			spdlog::error("(at line {})", line_idx + 1);
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
					spdlog::error("Error while parsing map \"{}\"", map);
					spdlog::error("Unknown caracter '{}' in map layout", line[column_idx]);
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
				spdlog::error("Internal error while parsing map \"{}\"", map);
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
		m.set_direction(view::facing_direction::E); // TODO
		m.set_pos(hitbox.center.x, hitbox.center.y);

		view_handle view_handle = view.acquire_overmap()->add_mob(std::move(m));
		model_handle model_handle{model_entity_handle};
		m_model2view[model_handle] = view_handle;
		m_view2model[view_handle]  = model_handle;

		++model_entity_handle;
	}

	for (const std::variant<activator, gate, autoshooter> &actor : actors) {

		utils::visitor visitor{[&](const activator &activator) {
			                       const float TOPLEFT_X = static_cast<float>(activator.pos.x) * model::cell_width;
			                       const float TOPLEFT_Y = static_cast<float>(activator.pos.y) * model::cell_height;

			                       switch (activator.type) {

				                       case activator_type::BUTTON: {
					                       world.grid[activator.pos.x][activator.pos.y].interaction_handle = {world.interactions.size()};
					                       world.interactions.push_back({model::interaction_kind::LIGHT_MANUAL,
					                                                     model::interactable_kind::BUTTON, world.buttons.size()});
					                       std::vector<model::button::target_t> mtargets;
					                       mtargets.reserve(activator.target_tiles.size());
					                       for (const point &pt : activator.target_tiles) {
						                       mtargets.push_back({pt.x, pt.y});
					                       }
					                       world.buttons.emplace_back(model::button{std::move(mtargets)});

					                       view::object o{};
					                       o.set_pos(TOPLEFT_X, TOPLEFT_Y);
					                       o.set_id(utils::resource_manager::object_id::button, m_state.resources);

					                       view_handle view_handle = view.acquire_overmap()->add_object(std::move(o));
					                       model_handle model_handle{world.buttons.size() - 1};
					                       m_model2view[model_handle] = view_handle;
					                       m_view2model[view_handle]  = model_handle;
					                       break;
				                       }
				                       case activator_type::INDUCTION_LOOP: {
					                       // TODO
					                       break;
				                       }
				                       case activator_type::INFRARED_LASER: {
					                       // TODO
					                       break;
				                       }
				                       case activator_type::NONE:
					                       [[fallthrough]];
				                       default:
					                       spdlog::error("Internal error while parsing map \"{}\"", map);
					                       break;
			                       }
		                       },
		                       [&](const gate &gate) {
			                       if (gate.closed) {
				                       world.grid[gate.pos.x][gate.pos.y].type = model::cell_type::CHASM;
				                       view_map[gate.pos.x][gate.pos.y]        = view::map::cell::abyss;
			                       }
			                       // TODO
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

bool adapter::adapter::load_map(const std::filesystem::path &path) noexcept {
	init_maps();
	std::string string_path = path.generic_string();

	std::shared_ptr<cpptoml::table> map_file;
	try {
		map_file = cpptoml::parse_file(string_path);
	}
	catch (const cpptoml::parse_exception &parse_exception) {
		spdlog::error("Failed to load map \"{}\": {}", string_path, parse_exception.what());
		return false;
	}

	auto version = map_file->get_qualified_as<std::string>("file.version");
	if (!version) {
		spdlog::error("Failed to load map \"{}\": unknown file format", string_path);
		return false;
	}

	if (*version == "1.0.0") {
		return load_map_v1_0_0(map_file, string_path);
	}

	spdlog::error("Unsupported version \"{}\" for map \"{}\"", *version, string_path);
	return false;
}

bool adapter::adapter::map_is_loaded() noexcept {
	view::viewer &view = state::access<adapter>::view(m_state);
	return !view.acquire_map()->m_cells.empty();
}

void adapter::adapter::update_map(size_t x, size_t y, model::cell_type new_cell) noexcept {
	cells_changed_since_last_update.emplace_back(x, y);

	view::viewer &view = state::access<adapter>::view(m_state);

	view::map::cell current_cell = view.acquire_map()->m_cells[x][y];
	view::map::cell output_cell;
	switch (new_cell) {
		case model::cell_type::WALL:
			[[fallthrough]];
		case model::cell_type::CHASM:
			output_cell = view::map::cell::abyss;
			break;
		case model::cell_type::GROUND:
			output_cell = view::map::cell::concrete_tile; // FIXME
	}
	view.acquire_map()->m_cells[x][y] = output_cell;
	spdlog::trace("Changed tile ({} ; {}) from {} to {}", x, y, static_cast<int>(current_cell),
	              static_cast<int>(output_cell)); // TODO: res_manager::to_string(cell)
}

void adapter::adapter::move_entity(model_handle handle, float new_x, float new_y) noexcept {
	view::viewer &view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(handle); it != m_model2view.end()) {
		view.acquire_overmap()->move_entity(it->second, new_x, new_y);
	}
	else {
		spdlog::error("Move request for unknown entity {}", handle.handle);
	}
}

void adapter::adapter::rotate_entity(model_handle handle, float new_rad) noexcept {
	view::viewer &view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(handle); it != m_model2view.end()) {
		spdlog::trace("Rotating view entity {} to a target angle of {}", it->first.handle, new_rad);
		view.acquire_overmap()->rotate_entity(it->second, view::facing_direction::from_angle(new_rad));
	}
	else {
		spdlog::error("Rotate request for unknown model entity {}", handle.handle);
	}
}

adapter::draw_request adapter::adapter::tooltip_for(view_handle entity) noexcept {
	model::world &world = state::access<adapter>::model(m_state).world;

	if (auto it = m_view2model.find(entity); it != m_view2model.end()) {
		auto &components = world.components;
		auto handle      = it->second.handle;

		if (entity.is_mob) {
			ImGui::BeginTooltip();
			if (components.health[handle]) {
				ImGui::Text("Current HP: %u", components.health[handle]->points);
			}
			if (components.decision[handle]) { // FIXME: this component is reset before this function call
				ImGui::Text("Current decision: %s", to_string(*components.decision[entity.handle]));
			}
			if (components.hitbox[handle]) {
				model::vec2 top_left     = components.hitbox[handle]->top_left();
				model::vec2 bottom_right = components.hitbox[handle]->bottom_right();
				ImGui::Text("Hitbox: (%f ; %f) to (%f ; %f)", top_left.x, top_left.y, bottom_right.x, bottom_right.y);
				ImGui::Text("Current angle: %f", components.hitbox[handle]->rad);
			}
			ImGui::EndTooltip();
		}
		else {
			// FIXME: why buttons and not something else ?
			// (code is assuming second.handle refers to a button)
			auto targets = world.buttons[it->second.handle].targets;
			request::coords_list list;
			for (const auto &target : targets) {
				ImGui::BeginTooltip();
				ImGui::Text("Button target: (%zu ; %zu)", target.column, target.row);
				ImGui::EndTooltip();
				list.coords.push_back({target.column, target.row});
			}
			return list;
		}
	}
	else {
		spdlog::error("Tried to fetch data for unknown view entity with handle {}", entity.handle);
	}
	return {};
}

size_t adapter::view_hhash::operator()(const view_handle &h) const noexcept {
	if (h.is_mob) {
		return h.handle;
	}
	return h.handle | 0xFF00u;
}
