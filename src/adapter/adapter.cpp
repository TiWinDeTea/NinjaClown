#include <cpptoml/cpptoml.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "bot/bot_api.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "model/grid_point.hpp"
#include "model/model.hpp"
#include "ninja_clown/api.h"
#include "state_holder.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"
#include "utils/scope_guards.hpp"
#include "view/game_viewer.hpp"
#include "view/view.hpp"

using fmt::literals::operator""_a;

namespace {
template <typename... Args>
[[nodiscard]] std::string tooltip_text_prefix(const utils::resource_manager &res, std::string_view key, char const *prefix, Args &&... args) {
	std::string_view fmt = res.tooltip_for(key);
	return prefix + fmt::format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
[[nodiscard]] std::string tooltip_text(const utils::resource_manager &res, std::string_view key, Args &&... args) {
	return tooltip_text_prefix(res, key, "", std::forward<Args>(args)...);
}
} // namespace

bool adapter::adapter::load_map(const std::filesystem::path &path) noexcept {
	if (map_is_loaded()) {
		state::access<adapter>::model(m_state).bot_end_level();
	}

	auto clear = [this] {
		state::access<adapter>::model(m_state).world.reset();
		m_target_handle.reset();

		m_target_handle.reset();
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
		state::access<adapter>::set_current_map_path(m_state, "");
	}
	else {
		state::access<adapter>::set_current_map_path(m_state, path);
		state::access<adapter>::model(m_state).bot_start_level(bot::ffi{});
	}
	return success;
}

bool adapter::adapter::map_is_loaded() noexcept {
	view::view &view = state::access<adapter>::view(m_state);
	return view.has_map();
}

void adapter::adapter::fire_activator(model_handle handle) noexcept {
	// Empty for now
}

void adapter::adapter::close_gate(model_handle gate) noexcept {
	auto it = m_model2view.find(gate);
	if (it == m_model2view.end()) {
		utils::log::error(m_state.resources(), "adapter.unknown_model_handle", "model_handle"_a = gate.handle,
		                  "operation"_a = "close gate");
	}
	else {
		state::access<adapter>::view(m_state).game().reveal(it->second);
	}
}

void adapter::adapter::open_gate(model_handle gate) noexcept {
	auto it = m_model2view.find(gate);
	if (it == m_model2view.end()) {
		utils::log::error(m_state.resources(), "adapter.unknown_model_handle", "model_handle"_a = gate.handle, "operation"_a = "open gate");
	}
	else {
		state::access<adapter>::view(m_state).game().hide(it->second);
	}
}

void adapter::adapter::update_map(const model::grid_point &target, model::cell_type  /*new_cell*/) noexcept {
	m_cells_changed_since_last_update.emplace_back(target);
}

void adapter::adapter::move_entity(model_handle entity, float new_x, float new_y) noexcept {
	view::view &view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(entity); it != m_model2view.end()) {
		view.game().move_entity(it->second, new_x, new_y);
		mark_entity_as_dirty(entity.handle);
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
		mark_entity_as_dirty(entity.handle);
		state::access<adapter>::view(m_state).game().hide(it->second);
	}
}

void adapter::adapter::rotate_entity(model_handle entity, float new_rad) noexcept {
	view::view &view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(entity); it != m_model2view.end()) {
		utils::log::trace(m_state.resources(), "adapter.trace.rotate_entity", "view_handle"_a = it->first.handle, "angle"_a = new_rad);
		view.game().rotate_entity(it->second, view::facing_direction::from_angle(new_rad));
		mark_entity_as_dirty(entity.handle);
	}
	else {
		utils::log::error(m_state.resources(), "adapter.unknown_model_handle", "model_handle"_a = entity.handle,
		                  "operation"_a = "rotate entity");
	}
}

void adapter::adapter::mark_entity_as_dirty(model::handle_t model_handle) noexcept {
	m_entities_changed_since_last_update.emplace_back(model_handle);
}

void adapter::adapter::clear_entities_changed_since_last_update() noexcept {
	m_entities_changed_since_last_update.clear();
}

const std::vector<std::size_t> &adapter::adapter::entities_changed_since_last_update() noexcept {
	return m_entities_changed_since_last_update;
}

void adapter::adapter::bot_log(bot_log_level level, const char *text) {
	switch (level) {
		case bot_log_level::BTRACE:
			spdlog::trace("[BOT] {}", text);
			break;
		case bot_log_level::BDEBUG:
			spdlog::debug("[BOT] {}", text);
			break;
		case bot_log_level::BINFO:
			spdlog::info("[BOT] {}", text);
			break;
		case bot_log_level::BWARN:
			spdlog::warn("[BOT] {}", text);
			break;
		case bot_log_level::BERROR:
			spdlog::error("[BOT] {}", text);
			break;
		case bot_log_level::BCRITICAL:
			spdlog::critical("[BOT] {}", text);
			break;
		default:
			spdlog::warn("DLL used unknown log level ({})", level);
			spdlog::info("[BOT] {}", text);
			break;
	}
}


adapter::draw_request adapter::adapter::tooltip_for(view_handle entity) noexcept {
	const model::world &world = state::access<adapter>::model(m_state).world;

	draw_request list;
	request::info info_req;

	// Tooltip for the objective (end of level) block
	if (entity == m_target_handle) {
		info_req.lines.emplace_back(tooltip_text(m_state.resources(), "adapter.objective"));
		list.emplace_back(std::move(info_req));
		return list;
	}

	// Retrieving model's handle
	auto it = m_view2model.find(entity);
	if (it == m_view2model.end()) {
		utils::log::warn(m_state.resources(), "adapter.unknown_view_entity", "view_handle"_a = entity.handle);
		return list;
	}

	if (entity.is_mob) {
		return tooltip_for_mob(it->second, world.components);
	}

	switch (it->second.type) {
		case model_handle::ACTIVATOR: {
			return tooltip_for_activator(it->second);
		}
		case model_handle::ACTIONABLE: {
			return tooltip_for_actionable(it->second, entity);
		}
		case model_handle::ENTITY:
			utils::log::warn(m_state.resources(), "adapter.non_coherent_entity", "handle"_a = it->second.handle);
			break;
	}

	return list;
}

adapter::draw_request adapter::adapter::tooltip_for_actionable(model_handle actionable, view_handle view_actionable) noexcept {
	assert(actionable.type == model_handle::ACTIONABLE);

	request::info info_req;
	auto target_name = m_view2name.find(view_actionable);
	if (target_name != m_view2name.end()) {
		info_req.lines.emplace_back(tooltip_text(m_state.resources(), "adapter.named_gate", "handle"_a = actionable.handle,
		                                         "name"_a = target_name->second));
	}
	else {
		info_req.lines.emplace_back(
		  tooltip_text(m_state.resources(), "adapter.nameless_gate", "handle"_a = actionable.handle));
		utils::log::warn(m_state.resources(), "adapter.name_not_found", "handle"_a = actionable.handle,
		                 "kind"_a = "actionable");
	}

	draw_request list;
	list.emplace_back(std::move(info_req));
	return list;
}

adapter::draw_request adapter::adapter::tooltip_for_activator(model_handle activator) noexcept {
	assert(activator.type == model_handle::ACTIVATOR);
	const model::world &world = state::access<adapter>::model(m_state).world;
	request::info info_req;
	draw_request list;

	info_req.lines.emplace_back(tooltip_text(m_state.resources(), "adapter.activator", "handle"_a = activator.handle));

	auto targets = world.activators[activator.handle].targets;
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
			info_req.lines.emplace_back(tooltip_text_prefix(m_state.resources(), "adapter.named_target", "\t",
			                                                "handle"_a = target, "name"_a = target_name));
		}
		else {
			info_req.lines.emplace_back(
			  tooltip_text_prefix(m_state.resources(), "adapter.nameless_target", "\t", "handle"_a = target));
			utils::log::warn(m_state.resources(), "adapter.name_not_found", "handle"_a = target,
			                 "kind"_a = "activator target");
		}

		const model::actionable::instance_data &target_data = world.actionables[target].data;
		list.emplace_back(request::coords{target_data.pos.x, target_data.pos.y});
	}

	list.emplace_back(std::move(info_req));
	return list;
}

adapter::draw_request adapter::adapter::tooltip_for_mob(model_handle mob, const model::components& components) noexcept {
	assert(mob.type == model_handle::ENTITY);
	draw_request list;
	request::info info_req;

	if (components.health[mob.handle]) {
		info_req.lines.emplace_back(
		  tooltip_text( m_state.resources(), "adapter.hp", "hp"_a = components.health[mob.handle]->points));
	}

	if (components.hitbox[mob.handle]) {
		const model::component::hitbox &hitbox = *components.hitbox[mob.handle];
		model::vec2 top_left                   = hitbox.top_left();
		model::vec2 bottom_right               = hitbox.bottom_right();

		info_req.lines.emplace_back(tooltip_text( m_state.resources(), "adapter.hitbox", "top_left_x"_a = top_left.x,
		                                         "top_left_y"_a = top_left.y, "bottom_right_x"_a = bottom_right.x,
		                                         "bottom_right_y"_a = bottom_right.y));
		info_req.lines.emplace_back(
		  tooltip_text( m_state.resources(), "adapter.position", "x"_a = hitbox.center.x, "y"_a = hitbox.center.y));
		info_req.lines.emplace_back(
		  tooltip_text( m_state.resources(), "adapter.angle", "angle"_a = hitbox.rad));
	}

	if (!info_req.lines.empty()) {
		list.emplace_back(std::move(info_req));
	}

	return list;
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
