#include <cpptoml.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "model/grid_point.hpp"
#include "model/model.hpp"
#include "ninja_clown/api.h"
#include "state_holder.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"
#include "utils/scope_guards.hpp"
#include "view/dialogs.hpp"
#include "view/viewer.hpp"

namespace {
template <typename... Args>
bool tooltip_text_prefix(utils::resource_manager &res, std::string_view key, char const *prefix, Args &&... args) {
	utils::optional<std::string_view> fmt = res.tooltip_for(key);
	if (fmt) {
		auto formatted = fmt::format(*fmt, std::forward<Args>(args)...);
		ImGui::Text("%s%s", prefix, formatted.c_str());
		return true;
	}

	std::string key_string{key};
	ImGui::Text("MISSING LOCALIZATION: %s", key_string.c_str());

	return false;
}

template <typename... Args>
bool tooltip_text(utils::resource_manager &res, std::string_view key, Args &&... args) {
	return tooltip_text_prefix(res, key, "", std::forward<Args>(args)...);
}
} // namespace

using fmt::literals::operator""_a;

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
		utils::log::error(m_state.resources(), "adapter.map_load_failure", "path"_a = string_path, "reason"_a = parse_exception.what());
		return false;
	}

	auto version = map_file->get_qualified_as<std::string>("file.version");
	if (!version) {
		utils::log::error(m_state.resources(), "adapter.map_load_failure", "path"_a = string_path,
		                  "reason"_a = "unknown file format (missing [file].version)");
		return false;
	}

	bool success{false};
	if (*version == "1.0.0") {
		success = load_map_v1_0_0(map_file, string_path);
	}

	if (!success) {
		utils::log::error(m_state.resources(), "adapter.unsupported_version", "path"_a = string_path, "version"_a = *version);
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
		utils::log::error(m_state.resources(), "adapter.unknown_model_handle", "model_handle"_a = gate.handle,
		                  "operation"_a = "close gate");
	}
	else {
		state::access<adapter>::view(m_state).acquire_overmap()->reveal(it->second);
	}
}

void adapter::adapter::open_gate(model_handle gate) noexcept {
	auto it = m_model2view.find(gate);
	if (it == m_model2view.end()) {
		utils::log::error(m_state.resources(), "adapter.unknown_model_handle", "model_handle"_a = gate.handle, "operation"_a = "open gate");
	}
	else {
		state::access<adapter>::view(m_state).acquire_overmap()->hide(it->second);
	}
}

void adapter::adapter::update_map(const model::grid_point &target, model::cell_type new_cell) noexcept {
	m_cells_changed_since_last_update.emplace_back(target);
}

void adapter::adapter::move_entity(model_handle entity, float new_x, float new_y) noexcept {
	view::viewer &view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(entity); it != m_model2view.end()) {
		view.acquire_overmap()->move_entity(m_state.resources(), it->second, new_x, new_y);
		m_entities_changed_since_last_update.emplace_back(entity.handle);
	}
	else {
		utils::log::error(m_state.resources(), "adapter.unknown_model_handle", "model_handle"_a = entity.handle,
		                  "operation"_a = "move entity");
	}
}

void adapter::adapter::hide_entity(model_handle entity) noexcept {
	auto it = m_model2view.find(entity);
	if (it == m_model2view.end()) {
		utils::log::error(m_state.resources(), "adapter.unknown_model_handle", "model_handle"_a = entity.handle,
		                  "operation"_a = "hide entity");
	}
	else {
		m_entities_changed_since_last_update.emplace_back(entity.handle);
		state::access<adapter>::view(m_state).acquire_overmap()->hide(it->second);
	}
}

void adapter::adapter::rotate_entity(model_handle entity, float new_rad) noexcept {
	view::viewer &view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(entity); it != m_model2view.end()) {
		utils::log::trace(m_state.resources(), "adapter.trace.rotate_entity", "view_handle"_a = it->first.handle, "angle"_a = new_rad);
		view.acquire_overmap()->rotate_entity(m_state.resources(), it->second, view::facing_direction::from_angle(new_rad));
		m_entities_changed_since_last_update.emplace_back(entity.handle);
	}
	else {
		utils::log::error(m_state.resources(), "adapter.unknown_model_handle", "model_handle"_a = entity.handle,
		                  "operation"_a = "rotate entity");
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

adapter::draw_request adapter::adapter::tooltip_for(view_handle entity) noexcept {
	model::world &world = state::access<adapter>::model(m_state).world;

	ImGui::BeginTooltip();
	ImGui::Separator();
	ON_SCOPE_EXIT {
		ImGui::Separator();
		ImGui::EndTooltip();
	};

	if (entity == m_target_handle) {
		tooltip_text(m_state.resources(), "adapter.objective");
		return {};
	}

	if (auto it = m_view2model.find(entity); it != m_view2model.end()) {
		auto &components = world.components;
		auto handle      = it->second.handle;

		if (entity.is_mob) {
			if (components.health[handle]) {
				tooltip_text(m_state.resources(), "adapter.hp", "hp"_a = components.health[handle]->points);
			}

			if (components.hitbox[handle]) {
				model::component::hitbox &hitbox = *components.hitbox[handle];
				model::vec2 top_left             = hitbox.top_left();
				model::vec2 bottom_right         = hitbox.bottom_right();
				tooltip_text(m_state.resources(), "adapter.hitbox", "top_left_x"_a = top_left.x, "top_left_y"_a = top_left.y,
				             "bottom_right_x"_a = bottom_right.x, "bottom_right_y"_a = bottom_right.y);
				tooltip_text(m_state.resources(), "adapter.position", "x"_a = hitbox.center.x, "y"_a = hitbox.center.y);
				tooltip_text(m_state.resources(), "adapter.angle", "angle"_a = components.hitbox[handle]->rad);
			}
		}
		else {
			auto model_it = m_view2model.find(entity);
			assert(it != m_view2model.end()); // NOLINT

			switch (model_it->second.type) {
				case model_handle::ACTIVATOR: {
					auto targets = world.activators[it->second.handle].targets;
					request::coords_list list;
					tooltip_text(m_state.resources(), "adapter.activator", "handle"_a = it->second.handle);
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
							tooltip_text_prefix(m_state.resources(), "adapter.named_target", "\t", "handle"_a = target,
							                    "name"_a = target_name);
						}
						else {
							tooltip_text_prefix(m_state.resources(), "adapter.nameless_target", "\t", "handle"_a = target);
							utils::log::warn(m_state.resources(), "adapter.name_not_found", "handle"_a = target,
							                 "kind"_a = "activator target");
						}
					}
					return list;
				}
				case model_handle::ACTIONABLE: {
					auto target_name = m_view2name.find(entity);
					if (target_name != m_view2name.end()) {
						tooltip_text(m_state.resources(), "adapter.named_gate", "handle"_a = it->second.handle,
						             "name"_a = target_name->second);
					}
					else {
						tooltip_text(m_state.resources(), "adapter.nameless_gate", "handle"_a = it->second.handle);
						utils::log::warn(m_state.resources(), "adapter.name_not_found", "handle"_a = model_it->second.handle,
						                 "kind"_a = "actionable");
					}
					break;
				}
				case model_handle::ENTITY:
					utils::log::warn(m_state.resources(), "adapter.non_coherent_entity", "handle"_a = model_it->second.handle);
					break;
			}
		}
	}
	else {
		utils::log::warn(m_state.resources(), "adapter.unknown_view_entity", "view_handle"_a = entity.handle);
	}
	return {};
}

void adapter::adapter::clear_cells_changed_since_last_update() noexcept {
	m_cells_changed_since_last_update.clear();
}

const std::vector<model::grid_point> &adapter::adapter::cells_changed_since_last_update() noexcept {
	return m_cells_changed_since_last_update;
}
utils::resource_manager &adapter::adapter::resources() {
    return m_state.resources();
}

std::size_t adapter::view_hhash::operator()(const view_handle &h) const noexcept {
	if (h.is_mob) {
		return h.handle;
	}
	return h.handle | 0xFF00u;
}
