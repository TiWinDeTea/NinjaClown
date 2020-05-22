#include <bot_interface/bot.h>
#include <cpptoml.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "state_holder.hpp"

bool adapter::adapter::load_map(const std::filesystem::path &path) noexcept {
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
		spdlog::error("Failed to load map \"{}\": unknown file format (missing [file].version)", string_path);
		return false;
	}

	if (*version == "1.0.0") {
		return load_map_v1_0_0(map_file, string_path);
	}

	spdlog::error(R"(Unsupported version "{}" for map "{}")", *version, string_path);
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
