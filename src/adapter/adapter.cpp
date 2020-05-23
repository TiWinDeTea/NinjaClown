#include <bot_interface/bot.h>
#include <cpptoml.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "model/grid_point.hpp"
#include "state_holder.hpp"

bool adapter::adapter::load_map(const std::filesystem::path &path) noexcept {
	auto clear = [this] {
        state::access<adapter>::model(m_state).world.reset();
        state::access<adapter>::view(m_state).acquire_overmap()->clear();
        state::access<adapter>::view(m_state).acquire_map()->m_cells.clear();
	};

	clear();
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

    bool success{false};
	if (*version == "1.0.0") {
		success = load_map_v1_0_0(map_file, string_path);
	}

	if (!success) {
        spdlog::error(R"(Unsupported version "{}" for map "{}")", *version, string_path);
		clear();
    }
	return success;
}

bool adapter::adapter::map_is_loaded() noexcept {
	view::viewer &view = state::access<adapter>::view(m_state);
	return !view.acquire_map()->m_cells.empty();
}

void adapter::adapter::close_gate(model_handle gate) noexcept {
	auto it = m_gates_model2view.find(gate);
	if (it == m_gates_model2view.end()) {
		// todo externalize
		spdlog::warn("Unknown handle encountered when trying to close gate.");
		spdlog::warn("Model handle value: {}", gate.handle);
	} else {
		state::access<adapter>::view(m_state).acquire_overmap()->reveal(it->second);
	}
}

void adapter::adapter::open_gate(model_handle gate) noexcept {
    auto it = m_gates_model2view.find(gate);
    if (it == m_gates_model2view.end()) {
        // todo externalize
        spdlog::warn("Unknown handle encountered when trying to close gate.");
        spdlog::warn("Model handle value: {}", gate.handle);
    } else {
        state::access<adapter>::view(m_state).acquire_overmap()->hide(it->second);
    }
}


void adapter::adapter::update_map(const model::grid_point &target, model::cell_type new_cell) noexcept {
	cells_changed_since_last_update.emplace_back(target);
}

void adapter::adapter::move_entity(model_handle handle, float new_x, float new_y) noexcept {
	view::viewer &view = state::access<adapter>::view(m_state);

	if (auto it = m_mobs_model2view.find(handle); it != m_mobs_model2view.end()) {
		view.acquire_overmap()->move_entity(it->second, new_x, new_y);
	}
	else {
		spdlog::error("Move request for unknown entity {}", handle.handle);
	}
}

void adapter::adapter::rotate_entity(model_handle handle, float new_rad) noexcept {
	view::viewer &view = state::access<adapter>::view(m_state);

	if (auto it = m_mobs_model2view.find(handle); it != m_mobs_model2view.end()) {
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
			auto targets = world.activators[it->second.handle].targets;
			request::coords_list list;
			for (size_t target : targets) {
				ImGui::BeginTooltip();
				ImGui::Text("Button target: %zu", target);
				ImGui::EndTooltip();
			}
			return list;
		}
	}
	else {
		spdlog::error("Tried to fetch data for unknown view entity with handle {}", entity.handle);
	}
	return {};
}

std::size_t adapter::view_hhash::operator()(const view_handle &h) const noexcept {
	if (h.is_mob) {
		return h.handle;
	}
	return h.handle | 0xFF00u;
}
