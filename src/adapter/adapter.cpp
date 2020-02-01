#include <fstream>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"

#include "model/cell.hpp"
#include "view/facing_dir.hpp"
#include "state_holder.hpp"

namespace {}

bool adapter::adapter::load_map(const std::filesystem::path &path) noexcept {
	if (!std::filesystem::is_regular_file(path)) {
		spdlog::error("Request to load map failed: \"{}\" is not a regular file.", path.generic_string());
		return false;
	}

	view::viewer &view   = state::access<adapter>::view(m_state);
	model::world &world  = state::access<adapter>::model(m_state).world;

	view.acquire_mobs()->clear();
	view.acquire_objects()->clear();

	std::ifstream fs{path};

	size_t width, height;
	fs >> width >> height;

	size_t next_entity_handle  = 0;
	bool ninja_clown_not_found = true;
	world.grid.resize(width);
	for (auto &column : world.grid) {
		column.resize(height);
	}

	for (size_t row = 0; row < height; ++row) {
		for (size_t column = 0; column < width; ++column) {
			model::cell &cell = world.grid[column][row];

			char type;
			fs >> std::noskipws >> type;
			if (type == '\n') {
				fs >> std::noskipws >> type;
			}

			switch (type) {
				case '#':
				case 'P':
					cell.type = model::cell_type::WALL;
					break;
				case 'b': {
					cell.type               = model::cell_type::GROUND;
					cell.interaction_handle = {world.interactions.size()};
					world.interactions.push_back(
					  {model::interaction_kind::LIGHT_MANUAL, model::interactable_kind::BUTTON, world.buttons.size()});
					world.buttons.push_back(model::button{4, 2});

					view::object o{};
					o.set_pos(static_cast<float>(column), static_cast<float>(row));
					auto obj_anim = m_state.resources.object_animation(utils::resource_manager::object_id::button);
					assert(obj_anim);
					o.set_animation(*obj_anim);
					auto objects = view.acquire_objects();
					objects->push_back(o);

					view_handle view{false, objects->size() - 1};
					model_handle model{world.buttons.size() - 1};
					m_model2view[model] = view;
					m_view2model[view]  = model;
					break;
				}
				case '@': {
					ninja_clown_not_found                             = false;
					world.ninja_clown_handle                          = next_entity_handle++;
					world.components.health[world.ninja_clown_handle] = {1};
					world.components.hitbox[world.ninja_clown_handle] = {static_cast<float>(column), static_cast<float>(row), 1.f, 1.f};
					world.components.angle[world.ninja_clown_handle]  = {0.f};

					view::mob m{};
					m.set_animations(m_state.resources.mob_animations(utils::resource_manager::mob_id::player).value());
					m.set_direction(view::facing_direction::E);
					m.set_pos(static_cast<float>(column), static_cast<float>(row));
					auto mobs = view.acquire_mobs();
					mobs->emplace_back(m);

					model_handle model{world.ninja_clown_handle};
					view_handle view{true, mobs->size() - 1};
					m_model2view[model] = view;
					m_view2model[view]  = model;
					cell.type           = model::cell_type::GROUND;
					break;
				}
				case 'D':
				case ' ':
					cell.type = model::cell_type::GROUND;
					break;
				default:
					cell.type = model::cell_type::CHASM;
					break;
			}
		}
	}

	if (ninja_clown_not_found) {
		spdlog::error("ninja clown not found in the map file!");
		return false;
	}

	std::vector<std::vector<view::map::cell>> view_map;
	view_map.resize(width);
	for (unsigned int x = 0; x < width; ++x) {
		view_map[x].resize(height);
		for (unsigned int y = 0; y < height; ++y) {
			switch (world.grid[x][y].type) {
				case model::cell_type::CHASM:
					[[fallthrough]];
				case model::cell_type::WALL:
					view_map[x][y] = view::map::cell::abyss;
					break;
				case model::cell_type::GROUND:
					view_map[x][y] = view::map::cell::concrete_tile;
			}
		}
	}
	view.set_map(std::move(view_map));

	spdlog::info("Loaded map \"{}\"", path.generic_string());
	return true;
}

void adapter::adapter::update_map(size_t x, size_t y, model::cell_type new_cell) noexcept {
	view::viewer& view = state::access<adapter>::view(m_state);

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
    view::viewer& view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(handle); it != m_model2view.end()) {
		if (it->second.is_mob) {
			spdlog::trace("Moving mob {} to ({} ; {})", handle.handle, new_x, new_y);
			(*view.acquire_mobs())[it->second.handle].set_pos(new_x, new_y);
		}
		else {
			spdlog::trace("Moving object {} to ({} ; {})", handle.handle, new_x, new_y);
			(*view.acquire_objects())[it->second.handle].set_pos(new_x, new_y);
		}
	}
	else {
		spdlog::error("Move request for unknown entity {}", handle.handle);
	}
}

void adapter::adapter::rotate_entity(model_handle handle, float new_rad) noexcept {
    view::viewer& view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(handle); it != m_model2view.end()) {
		if (it->second.is_mob) {
			spdlog::trace("Rotating mob {} to a target angle of {}", it->first.handle, new_rad);
			(*view.acquire_mobs())[it->second.handle].set_direction(view::facing_direction::from_angle(new_rad));
		}
		else {
			spdlog::error("Rotate request for non-compatible object {}", handle.handle);
		}
	}
	else {
		spdlog::error("Rotate request for unknown model entity {}", handle.handle);
	}
}

adapter::draw_request adapter::adapter::tooltip_for(view_handle entity) noexcept {
	model::world& world = state::access<adapter>::model(m_state).world;

	if (auto it = m_view2model.find(entity); it != m_view2model.end()) {
		auto &components = world.components;
		auto handle      = it->second.handle;

		if (entity.is_mob) {
			ImGui::BeginTooltip();
			if (components.health[handle]) {
				ImGui::Text("Current HP: %u", components.health[handle]->points);
			}
			if (components.angle[handle]) {
				ImGui::Text("Current angle: %f", components.angle[handle]->rad);
			}
			if (components.decision[handle]) {
				ImGui::Text("Current decision: %s", to_string(*components.decision[entity.handle]));
			}
			if (components.hitbox[handle]) {
				ImGui::Text("Hitbox: (%f ; %f) to (%f ; %f)", components.hitbox[handle]->x, components.hitbox[handle]->y,
				            components.hitbox[handle]->right_x(), components.hitbox[handle]->bottom_y());
			}
			ImGui::EndTooltip();

			if (components.hitbox[handle]) {
				const auto &box = *components.hitbox[handle];
				return request::hitbox{box.x, box.y, box.width, box.height};
			}
		}
		else {
			ImGui::BeginTooltip();
			// FIXME: why buttons and not something else ?
			auto target = world.buttons[it->second.handle].target;
			ImGui::Text("Button target: (%zu ; %zu)", target.column, target.row);
			ImGui::EndTooltip();
			return request::coords{target.column, target.row};
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
