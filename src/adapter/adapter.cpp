#include <bot_interface/bot.h>
#include <cpptoml.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "model/grid_point.hpp"
#include "state_holder.hpp"
#include "utils/scope_guards.hpp"
#include "view/dialogs.hpp"
#include "view/viewer.hpp"

bool adapter::adapter::load_map(const std::filesystem::path &path) noexcept {
	auto clear = [this] {
		state::access<adapter>::model(m_state).world.reset();
		state::access<adapter>::view(m_state).acquire_overmap()->clear();
		state::access<adapter>::view(m_state).acquire_map()->m_cells.clear();
		state::access<adapter>::view(m_state).dialog_viewer().clear(); // TODO: mutex pour ici ?
		m_target_handle.reset();

		m_target_handle.reset();
		m_model2dialog.clear();
		m_model2view.clear();
		m_view2model.clear();
		m_view2name.clear();
		m_cells_changed_since_last_update.clear();
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

void adapter::adapter::fire_activator(model_handle handle) noexcept {
	auto it = m_model2dialog.find(handle);
	if (it != m_model2dialog.end()) {
		state::access<adapter>::view(m_state).dialog_viewer().select_dialog(it->second);
	}
}

void adapter::adapter::close_gate(model_handle gate) noexcept {
	auto it = m_model2view.find(gate);
	if (it == m_model2view.end()) {
		// todo externalize
		spdlog::warn("Unknown handle encountered when trying to close gate.");
		spdlog::warn("Model handle value: {}", gate.handle);
	}
	else {
		state::access<adapter>::view(m_state).acquire_overmap()->reveal(it->second);
	}
}

void adapter::adapter::open_gate(model_handle gate) noexcept {
	auto it = m_model2view.find(gate);
	if (it == m_model2view.end()) {
		// todo externalize
		spdlog::warn("Unknown handle encountered when trying to close gate.");
		spdlog::warn("Model handle value: {}", gate.handle);
	}
	else {
		state::access<adapter>::view(m_state).acquire_overmap()->hide(it->second);
	}
}

void adapter::adapter::update_map(const model::grid_point &target, model::cell_type new_cell) noexcept {
	m_cells_changed_since_last_update.emplace_back(target);
}

void adapter::adapter::move_entity(model_handle handle, float new_x, float new_y) noexcept {
	view::viewer &view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(handle); it != m_model2view.end()) {
		view.acquire_overmap()->move_entity(it->second, new_x, new_y);
		m_entities_changed_since_last_update.emplace_back(handle.handle);
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
		m_entities_changed_since_last_update.emplace_back(handle.handle);
	}
	else {
		spdlog::error("Rotate request for unknown model entity {}", handle.handle);
	}
}

void adapter::adapter::clear_entities_changed_since_last_update() noexcept {
	m_entities_changed_since_last_update.clear();
}

const std::vector<std::size_t> &adapter::adapter::entities_changed_since_last_update() noexcept {
	return m_entities_changed_since_last_update;
}

void adapter::adapter::dll_log(const char *log) {
	view::dialog_viewer &dialog = state::access<adapter>::view(m_state).dialog_viewer();
	dialog.dll_word(log, std::chrono::milliseconds{2500}); // TODO : externalize/parameterify
	spdlog::info("BOT LOG: {}", log);
}

// TODO traductions
adapter::draw_request adapter::adapter::tooltip_for(view_handle entity) noexcept {

	model::world &world = state::access<adapter>::model(m_state).world;

	ImGui::BeginTooltip();
	ImGui::Separator();
	ON_SCOPE_EXIT {
		ImGui::Separator();
		ImGui::EndTooltip();
	};

	if (entity == m_target_handle) {
		ImGui::Text("Your objective.");
		return {};
	}

	if (auto it = m_view2model.find(entity); it != m_view2model.end()) {
		auto &components = world.components;
		auto handle      = it->second.handle;

		if (entity.is_mob) {
			if (components.health[handle]) {
				ImGui::Text("Current HP: %u", components.health[handle]->points);
			}

			if (components.hitbox[handle]) {
				model::component::hitbox &hitbox = *components.hitbox[handle];
				model::vec2 top_left             = hitbox.top_left();
				model::vec2 bottom_right         = hitbox.bottom_right();
				ImGui::Text("Hitbox: (%f ; %f) to (%f ; %f)", top_left.x, top_left.y, bottom_right.x, bottom_right.y);
				ImGui::Text("Position: (%f ; %f)", hitbox.center.x, hitbox.center.y);
				ImGui::Text("Current angle: %f", components.hitbox[handle]->rad);
			}
		}
		else {
			auto model_it = m_view2model.find(entity);
			assert(it != m_view2model.end()); // NOLINT

			switch (model_it->second.type) {
				case model_handle::ACTIVATOR: {
					auto targets = world.activators[it->second.handle].targets;
					request::coords_list list;
					ImGui::Text("Activator %zu", it->second.handle);
					for (size_t target : targets) {
						std::string target_name;
						auto target_view_handle = m_model2view.find(model_handle{target, model_handle::ACTIONABLE});
						if (target_view_handle != m_model2view.end()) {
							auto target_name_it = m_view2name.find(target_view_handle->second);
							if (target_name_it != m_view2name.end()) {
								target_name = target_name_it->second;
							}
						}

						if (!target_name.empty()) {
							ImGui::Text("    target: %zu, %s", target, target_name.c_str());
						}
						else {
							ImGui::Text("    target: %zu", target);
							spdlog::warn("Name not found for activator target {}", target);
						}
					}
					return list;
				}
				case model_handle::ACTIONABLE: {
					auto target_name = m_view2name.find(entity);
					if (target_name != m_view2name.end()) {
						ImGui::Text("Gate : %zu, %s", model_it->second.handle, target_name->second.c_str());
					}
					else {
						spdlog::warn("Name not found for actionable {}", model_it->second.handle);
						ImGui::Text("Gate : %zu", model_it->second.handle);
					}
					break;
				}
				case model_handle::ENTITY:
					spdlog::warn("Non coherent entity type, logic and view might be out of sync (internal error)");
					break;
			}
		}
	}
	else {
		spdlog::warn("Tried to fetch data for unknown view entity with handle {}", entity.handle);
		spdlog::warn("logic and view might be out of sync (internal error)");
	}
	return {};
}

void adapter::adapter::clear_cells_changed_since_last_update() noexcept {
	m_cells_changed_since_last_update.clear();
}

const std::vector<model::grid_point> &adapter::adapter::cells_changed_since_last_update() noexcept {
	return m_cells_changed_since_last_update;
}

std::size_t adapter::view_hhash::operator()(const view_handle &h) const noexcept {
	if (h.is_mob) {
		return h.handle;
	}
	return h.handle | 0xFF00u;
}
