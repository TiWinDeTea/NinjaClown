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
#include "utils/visitor.hpp"
#include "view/game/game_viewer.hpp"
#include "view/view.hpp"

using fmt::literals::operator""_a;

namespace {

adapter::behaviour::bhvr nnj_ett_kind2adapter_behaviour(ninja_api::nnj_entity_kind ett_kind) {
	switch (ett_kind) {
		case ninja_api::EK_NOT_AN_ENTITY:
			return adapter::behaviour::bhvr::unsupported;
		case ninja_api::EK_HARMLESS:
			return adapter::behaviour::bhvr::harmless;
		case ninja_api::EK_PATROL:
			return adapter::behaviour::bhvr::patrol;
		case ninja_api::EK_AGGRESSIVE:
			return adapter::behaviour::bhvr::aggressive;
		case ninja_api::EK_DLL:
			return adapter::behaviour::bhvr::dll;
		case ninja_api::EK_PROJECTILE:
		default:
			return adapter::behaviour::bhvr::unsupported;
	}
}

ninja_api::nnj_entity_kind adapter_behaviour2nnj_ett_kind(adapter::behaviour::bhvr behaviour) {
	switch (behaviour) {
		case adapter::behaviour::bhvr::harmless:
			return ninja_api::EK_HARMLESS;
		case adapter::behaviour::bhvr::patrol:
			return ninja_api::EK_PATROL;
		case adapter::behaviour::bhvr::aggressive:
			return ninja_api::EK_AGGRESSIVE;
		case adapter::behaviour::bhvr::dll:
			return ninja_api::EK_DLL;
		case adapter::behaviour::bhvr::unsupported:
		default:
			return ninja_api::EK_NOT_AN_ENTITY;
	}
}

template <typename... Args>
[[nodiscard]] std::string tooltip_text_prefix(std::string_view key, char const *prefix, Args &&...args) {
	std::string_view fmt = utils::resource_manager::instance().tooltip_for(key);
	return prefix + fmt::format(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
[[nodiscard]] std::string tooltip_text(std::string_view key, Args &&...args) {
	return tooltip_text_prefix(key, "", std::forward<Args>(args)...);
}

std::pair<adapter::model_handle, adapter::view_handle> add_button(unsigned int x, unsigned int y, model::world &world, view::view &view) {
	// TODO check if (x,y) is already occupied
	const float topleft_x = static_cast<float>(x) * model::cst::cell_width;
	const float topleft_y = static_cast<float>(y) * model::cst::cell_height;

	world.map[x][y].interaction_handle = {world.interactions.size()};

	view::object obj{};
	obj.set_pos(topleft_x, topleft_y);
	obj.set_id(utils::resources_type::object_id::button);

	world.interactions.push_back({model::interaction_kind::LIGHT_MANUAL, model::interactable_kind::BUTTON, world.activators.size()});
	world.activators.push_back(
	  {{}, {std::numeric_limits<model::tick_t>::max()}, model::default_activation_delay, model::default_activation_difficulty, false});

	return {adapter::model_handle{world.activators.size() - 1, adapter::model_handle::ACTIVATOR}, view.add_object(std::move(obj))};
}

std::pair<adapter::model_handle, adapter::view_handle> add_gate(unsigned int x, unsigned int y, model::world &world, view::view &view) {
	// TODO check if (x,y) is already occupied

	std::size_t id = 0u;
	for (const auto &actionable : world.actionables) {
		id = std::max(id, actionable.data.handle + 1);
	}

	world.actionables.push_back({{{x, y}, id}, model::actionable::behaviours_ns::gate});

	view::object vgate{};
	vgate.set_pos(static_cast<float>(x) * model::cst::cell_width, static_cast<float>(y) * model::cst::cell_height);
	vgate.set_id(utils::resources_type::object_id::gate);

	adapter::view_handle view_handle = view.add_object(std::move(vgate));
	adapter::model_handle model_handle{id, adapter::model_handle::ACTIONABLE};

	// Door closed by default
	world.map[x][y].type = model::cell_type::CHASM;
	return {model_handle, view_handle};
}

std::pair<adapter::model_handle, adapter::view_handle> add_autoshooter(unsigned int x, unsigned int y, model::world &world,
                                                                       view::view &view) {
	// TODO check if (x,y) is already occupied
	// TODO
	return {adapter::model_handle{}, adapter::view_handle{}};
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
		m_name2view.clear();
		m_cells_changed_since_last_update.clear();
	};

	clear();
	std::string string_path = path.generic_string();

	std::shared_ptr<cpptoml::table> map_file;
	try {
		map_file = cpptoml::parse_file(string_path);
	}
	catch (const cpptoml::parse_exception &parse_exception) {
		utils::log::error("adapter.map_load_failure", "path"_a = string_path, "reason"_a = parse_exception.what());
		return false;
	}

	auto version = map_file->get_qualified_as<std::string>("file.version");
	if (!version) {
		utils::log::error("adapter.map_load_failure", "path"_a = string_path, "reason"_a = "unknown file format (missing [file].version)");
		return false;
	}

	bool success{false};
	if (*version == "1.0.0") {
		success = load_map_v1_0_0(map_file, string_path);
	}

	if (!success) {
		utils::log::error("adapter.unsupported_version", "path"_a = string_path, "version"_a = *version);
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
	const view::view &view = state::access<adapter>::view(m_state);
	return view.has_map();
}

void adapter::adapter::fire_activator(model_handle handle) noexcept {
	// Empty for now
	// TODO
}

void adapter::adapter::close_gate(model_handle gate) noexcept {
	auto it = m_model2view.find(gate);
	if (it == m_model2view.end()) {
		utils::log::error("adapter.unknown_model_handle", "model_handle"_a = gate.handle, "operation"_a = "close gate");
	}
	else {
		state::access<adapter>::view(m_state).reveal(it->second);
	}
}

void adapter::adapter::open_gate(model_handle gate) noexcept {
	auto it = m_model2view.find(gate);
	if (it == m_model2view.end()) {
		utils::log::error("adapter.unknown_model_handle", "model_handle"_a = gate.handle, "operation"_a = "open gate");
	}
	else {
		state::access<adapter>::view(m_state).hide(it->second);
	}
}

void adapter::adapter::update_map(const model::grid_point &target, model::cell_type /*new_cell*/) noexcept {
	m_cells_changed_since_last_update.emplace_back(target);
}

void adapter::adapter::move_entity(model_handle entity, float new_x, float new_y) noexcept {
	view::view &view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(entity); it != m_model2view.end()) {
		view.move_entity(it->second, new_x, new_y);
		mark_entity_as_dirty(entity.handle);
	}
	else {
		utils::log::error("adapter.unknown_model_handle", "model_handle"_a = entity.handle, "operation"_a = "move entity");
	}
}

void adapter::adapter::hide_entity(model_handle entity) noexcept {
	auto it = m_model2view.find(entity);
	if (it == m_model2view.end()) {
		utils::log::error("adapter.unknown_model_handle", "model_handle"_a = entity.handle, "operation"_a = "hide entity");
	}
	else {
		mark_entity_as_dirty(entity.handle);
		state::access<adapter>::view(m_state).hide(it->second);
	}
}

void adapter::adapter::rotate_entity(model_handle entity, float new_rad) noexcept {
	view::view &view = state::access<adapter>::view(m_state);

	if (auto it = m_model2view.find(entity); it != m_model2view.end()) {
		utils::log::trace("adapter.trace.rotate_entity", "view_handle"_a = it->first.handle, "angle"_a = new_rad);
		view.rotate_entity(it->second, view::facing_direction::from_angle(new_rad));
		mark_entity_as_dirty(entity.handle);
	}
	else {
		utils::log::error("adapter.unknown_model_handle", "model_handle"_a = entity.handle, "operation"_a = "rotate entity");
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
		info_req.lines.emplace_back(tooltip_text("adapter.objective"));
		list.emplace_back(std::move(info_req));
		return list;
	}

	// Retrieving model's handle
	auto it = m_view2model.find(entity);
	if (it == m_view2model.end()) {
		utils::log::warn("adapter.unknown_view_entity", "view_handle"_a = entity.handle, "is_mob"_a = entity.is_mob);
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
			utils::log::warn("adapter.non_coherent_entity", "handle"_a = it->second.handle);
			break;
	}

	return list;
}

adapter::draw_request adapter::adapter::tooltip_for_actionable(model_handle actionable, view_handle view_actionable) noexcept {
	assert(actionable.type == model_handle::ACTIONABLE);

	request::info info_req;
	auto target_name = m_view2name.find(view_actionable);
	if (target_name != m_view2name.end()) {
		info_req.lines.emplace_back(tooltip_text("adapter.named_gate", "handle"_a = actionable.handle, "name"_a = target_name->second));
	}
	else {
		info_req.lines.emplace_back(tooltip_text("adapter.nameless_gate", "handle"_a = actionable.handle));
		utils::log::warn("adapter.name_not_found", "handle"_a = actionable.handle, "kind"_a = "actionable");
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

	info_req.lines.emplace_back(tooltip_text("adapter.activator", "handle"_a = activator.handle));

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
			info_req.lines.emplace_back(tooltip_text_prefix("adapter.named_target", "\t", "handle"_a = target, "name"_a = target_name));
		}
		else {
			info_req.lines.emplace_back(tooltip_text_prefix("adapter.nameless_target", "\t", "handle"_a = target));
			utils::log::warn("adapter.name_not_found", "handle"_a = target, "kind"_a = "activator target");
		}

		const model::actionable::instance_data &target_data = world.actionables[target].data;
		list.emplace_back(request::coords{target_data.pos.x, target_data.pos.y});
	}

	list.emplace_back(std::move(info_req));
	return list;
}

adapter::draw_request adapter::adapter::tooltip_for_mob(model_handle mob, const model::components &components) noexcept {
	assert(mob.type == model_handle::ENTITY);
	draw_request list;
	request::info info_req;

	if (components.health[mob.handle]) {
		info_req.lines.emplace_back(tooltip_text("adapter.hp", "hp"_a = components.health[mob.handle]->points));
	}

	if (components.hitbox[mob.handle]) {
		const model::component::hitbox &hitbox = *components.hitbox[mob.handle];
		model::vec2 top_left                   = hitbox.top_left();
		model::vec2 bottom_right               = hitbox.bottom_right();

		info_req.lines.emplace_back(tooltip_text("adapter.hitbox", "top_left_x"_a = top_left.x, "top_left_y"_a = top_left.y,
		                                         "bottom_right_x"_a = bottom_right.x, "bottom_right_y"_a = bottom_right.y));
		info_req.lines.emplace_back(tooltip_text("adapter.position", "x"_a = hitbox.center.x, "y"_a = hitbox.center.y));
		info_req.lines.emplace_back(tooltip_text("adapter.angle", "angle"_a = hitbox.rad));
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

void adapter::adapter::create_map(unsigned int width, unsigned int height) {
	model::world &world = state::access<adapter>::model(m_state).world;
	world.reset();
	m_model2view.clear();
	m_view2name.clear();
	m_view2model.clear();

	world.map.resize(width, height);

	view::map_viewer map_viewer{m_state};
	map_viewer.set_map({width, {height, {view::map::cell::abyss}}});
	state::access<adapter>::view(m_state).set_map(std::move(map_viewer));
}

void adapter::adapter::edit_tile(unsigned int x, unsigned int y, utils::resources_type::tile_id new_value) {
	model::world &world = state::access<adapter>::model(m_state).world;

	switch (new_value) {
		case utils::resources_type::tile_id::chasm:
			world.map[x][y].type = model::cell_type::CHASM;
			break;
		case utils::resources_type::tile_id::iron:
			// fallthrough
		case utils::resources_type::tile_id::concrete:
			world.map[x][y].type = model::cell_type::GROUND;
			break;
		case utils::resources_type::tile_id::frame:
			// fallthrough
		default:
			utils::log::warn("adapter.adapter.unknown_tile_type", "id"_a = static_cast<int>(new_value));
			return;
	}

	state::access<adapter>::view(m_state).set_tile(x, y, new_value);
}

void adapter::adapter::add_mob(unsigned int x, unsigned int y, utils::resources_type::mob_id id) {
	// TODO : check if (x,y) is already occupied

	model::world &world              = state::access<adapter>::model(m_state).world;
	unsigned int model_entity_handle = std::numeric_limits<unsigned int>::max();
	for (int i = 0; i < model::cst::max_entities; ++i) {
		if (world.components.metadata[i].kind == ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY) {
			model_entity_handle = i;
			break;
		}
	}

	if (model_entity_handle == std::numeric_limits<unsigned int>::max()) {
		utils::log::warn("adapter.adapter.too_many_entities", "max"_a = model::cst::max_entities);
		return;
	}

	const float center_x = static_cast<float>(x) * model::cst::cell_width + model::cst::cell_width / 2.f;
	const float center_y = static_cast<float>(y) * model::cst::cell_height + model::cst::cell_height / 2.f;

	switch (id) {
		case utils::resources_type::mob_id::player:
			world.components.metadata[model_entity_handle].kind = ninja_api::nnj_entity_kind::EK_DLL;
			break;
		case utils::resources_type::mob_id::scientist:
			world.components.metadata[model_entity_handle].kind = ninja_api::nnj_entity_kind::EK_HARMLESS;
			break;
		default:
			utils::log::warn("adapter.adapter.unknown_mob_type", "id"_a = static_cast<int>(id));
			return;
	}

	world.components.health[model_entity_handle] = {1};
	world.components.hitbox[model_entity_handle] = {center_x, center_y, 0.25f, 0.25f}; // FIXME : hardcoded 1/4.

	const model::component::hitbox &hitbox = *world.components.hitbox[model_entity_handle];
	view::mob mob{};
	mob.set_mob_id(id);
	mob.set_direction(view::facing_direction::from_angle(world.components.hitbox[model_entity_handle]->rad));
	mob.set_pos(hitbox.center.x, hitbox.center.y);

	const view_handle view_handle = state::access<adapter>::view(m_state).add_mob(std::move(mob));
	const model_handle model_handle{model::handle_t{model_entity_handle}, model_handle::ENTITY};
	m_model2view.insert({model_handle, view_handle});
	m_view2model.insert({view_handle, model_handle});
}

void adapter::adapter::add_object(unsigned int x, unsigned int y, utils::resources_type::object_id id) {

	switch (id) {

		case utils::resources_type::object_id::button: {
			auto handles = add_button(x, y, state::access<adapter>::model(m_state).world, state::access<adapter>::view(m_state));
			m_model2view[handles.first]  = handles.second;
			m_view2model[handles.second] = handles.first;
			break;
		}
		case utils::resources_type::object_id::gate: {
			auto handles = add_gate(x, y, state::access<adapter>::model(m_state).world, state::access<adapter>::view(m_state));
			m_model2view[handles.first]  = handles.second;
			m_view2model[handles.second] = handles.first;

			m_view2name[handles.second] = "gate" + std::to_string(handles.first.handle);
			break;
		}
		case utils::resources_type::object_id::autoshooter: {
			auto handles = add_autoshooter(x, y, state::access<adapter>::model(m_state).world, state::access<adapter>::view(m_state));
			m_model2view[handles.first]  = handles.second;
			m_view2model[handles.second] = handles.first;
			break;
		}
		case utils::resources_type::object_id::target: {
			model::world &world = state::access<adapter>::model(m_state).world;
			world.target_tile   = {x, y};

			if (m_target_handle) {
				state::access<adapter>::view(m_state).erase(*m_target_handle);
			}

			view::object obj;
			obj.set_id(utils::resources_type::object_id::target);
			obj.set_pos(static_cast<float>(x) * model::cst::cell_width, static_cast<float>(y) * model::cst::cell_height);
			obj.reveal();
			m_target_handle = state::access<adapter>::view(m_state).add_object(std::move(obj));
			break;
		}
		default:
			utils::log::warn("adapter.adapter.unknown_object", "id"_a = static_cast<int>(id)); // FIXME add key in lang files
	}
}

void adapter::adapter::remove_entity(view_handle view_handle) {

	if (view_handle == m_target_handle) {
		model::world &world = state::access<adapter>::model(m_state).world;
		world.target_tile.x = std::numeric_limits<decltype(world.target_tile.x)>::max();
		world.target_tile.y = std::numeric_limits<decltype(world.target_tile.y)>::max();

		state::access<adapter>::view(m_state).erase(view_handle);

		return;
	}

	try {
		auto model_handle = m_view2model.at(view_handle);
		m_model2view.erase(model_handle);
		m_view2model.erase(view_handle);

		state::access<adapter>::view(m_state).erase(view_handle);

		model::world &world = state::access<adapter>::model(m_state).world;
		switch (model_handle.type) {
			case model_handle::ACTIVATOR:
				// TODO
				break;
			case model_handle::ACTIONABLE: {
				// FIXME: check if activators work with ID (as coded) or with index (this section to reconsider)
				unsigned int index = 0u;
				while (index < world.actionables.size() && model_handle.handle != world.actionables[index].data.handle) {
					++index;
				}
				if (index < world.actionables.size()) {
					std::swap(world.actionables[index], world.actionables.back());
					world.actionables.erase(std::prev(world.actionables.end()));

					for (auto &activator : world.activators) {
						auto rm_begin = std::remove(activator.targets.begin(), activator.targets.end(), model_handle.handle);
						activator.targets.erase(rm_begin, activator.targets.end());
					}
				}
				else {
					utils::log::warn("adapter.adapter.remove_entity.unknown_actionable_handle",
					                 "handle"_a = static_cast<int>(model_handle.type));
				}
				break;
			}
			case model_handle::ENTITY:
				world.components.metadata[model_handle.handle].kind
				  = ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY; // TODO check if this is enough
				break;
			default:
				utils::log::warn("adapter.adapter.remove_entity.unknown_handle_type", "handle"_a = static_cast<int>(model_handle.type));
		}
	}
	catch (const std::out_of_range &e) {
		utils::log::warn("adapter.adapter.remove_entity.unknown_handle", "handle"_a = static_cast<int>(view_handle.handle),
		                 "is_mob"_a = view_handle.is_mob);
	}
}

void adapter::adapter::edit_entity(view_handle handle, const entity_edit &new_data) {

	if (handle == m_target_handle) {
		utils::log::warn("adapter.adapter.edit_entity.target_tile_edit");
		return;
	}

	// Retrieving model's handle
	auto it = m_view2model.find(handle);
	if (it == m_view2model.end()) {
		utils::log::warn("adapter.unknown_view_entity", "view_handle"_a = handle.handle, "is_mob"_a = handle.is_mob);
		return;
	}

	if (handle.is_mob) {
		edit_mob_entity(handle, new_data);
	}

	switch (it->second.type) {
		case model_handle::ACTIVATOR: {
			edit_activator_entity(handle, new_data);
			break;
		}
		case model_handle::ACTIONABLE: {
			edit_actionable_entity(handle, new_data);
			break;
		}
		case model_handle::ENTITY:
			utils::log::warn("adapter.non_coherent_entity", "handle"_a = it->second.handle);
			break;
	}
}

adapter::entity_edit adapter::adapter::entity_properties(view_handle handle) const {
	if (handle == m_target_handle) {
		return {};
	}

	// Retrieving model's handle
	auto it = m_view2model.find(handle);
	if (it == m_view2model.end()) {
		utils::log::warn("adapter.unknown_view_entity", "view_handle"_a = handle.handle, "is_mob"_a = handle.is_mob);
		return {};
	}

	if (handle.is_mob) {
		return mob_entity_properties(handle, it->second);
	}

	switch (it->second.type) {
		case model_handle::ACTIVATOR: {
			return activator_entity_properties(handle, it->second);
		}
		case model_handle::ACTIONABLE: {
			return actionable_entity_properties(handle, it->second);
		}
		case model_handle::ENTITY:
			utils::log::warn("adapter.non_coherent_entity", "handle"_a = it->second.handle);
			break;
	}

	return {};
}

std::size_t adapter::view_hhash::operator()(const view_handle &h) const noexcept {
	if (h.is_mob) {
		return h.handle;
	}
	return h.handle | 0xFF00u;
}

adapter::entity_edit adapter::adapter::actionable_entity_properties(view_handle vhandle, model_handle mhandle) const {
	entity_edit edit;
	const entity_edit_v name = m_view2name.at(vhandle);
	edit.emplace_back(utils::gui_str_for("adapter.properties.actionable.name"), name);
	// TODO : add door default value (closed or open)
	// TODO : autoshooter, ...
	return edit;
}

adapter::entity_edit adapter::adapter::activator_entity_properties(view_handle, model_handle mhandle) const {
	entity_edit edits;

	const auto &world = state::access<adapter>::model(m_state).world;

	std::vector<std::string> names;
	std::basic_string<bool> bools;
	for (const auto &handle_name : m_view2name) {

		auto &current_actionable = m_view2model.at(handle_name.first);
		bool is_target{false};
		for (size_t target : world.activators[mhandle.handle].targets) {
			if (target == current_actionable.handle) {
				is_target = true;
				break;
			}
		}
		bools.push_back(is_target);
		names.emplace_back(handle_name.second);
	}

	edits.emplace_back(utils::gui_str_for("adapter.properties.activator.link_to"),
	                   entity_edit_v{toggler_targets{std::move(names), std::move(bools)}});
	return edits;
}

adapter::entity_edit adapter::adapter::mob_entity_properties(view_handle vhandle, model_handle mhandle) const {
	// TODO: add behaviour selector

	entity_edit edit;
	auto &world = state::access<adapter>::model(m_state).world;

	const auto &hp_component = world.components.health[mhandle.handle];
	if (!hp_component) {
		utils::log::error("adapter.non_coherent_entity", "handle"_a = mhandle.handle);
		return {};
	}

	const auto &dir_component = world.components.hitbox[mhandle.handle];
	if (!dir_component) {
		utils::log::error("adapter.non_coherent_entity", "handle"_a = mhandle.handle);
		return {};
	}

	const entity_edit_v hp   = hp_component->points;
	const entity_edit_v dir  = angle{dir_component->rad};
	const entity_edit_v bhvr = behaviour{nnj_ett_kind2adapter_behaviour(world.components.metadata[mhandle.handle].kind)};

	edit.emplace_back(utils::gui_str_for("adapter.properties.mob.hp"), hp);
	edit.emplace_back(utils::gui_str_for("adapter.properties.mob.direction"), dir);
	edit.emplace_back(utils::gui_str_for("adapter.properties.mob.behaviour"), bhvr);
	return edit;
}

void adapter::adapter::edit_actionable_entity(view_handle handle, const entity_edit &edits) {
	auto visitor = [&](const auto &value) {
		using T = std::remove_cv_t<std::remove_reference_t<decltype(value)>>;
		if constexpr (std::is_same_v<T, std::string>) {
			m_view2name[handle] = value;
			m_name2view[value]  = handle;
		}
		else {
			utils::log::error("adapter.properties.bad_property");
		}
	};

	for (const auto &edit : edits) {
		std::visit(visitor, edit.second);
	}
}

void adapter::adapter::edit_activator_entity(view_handle handle, const entity_edit &edits) {
	utils::visitor visitor{

	  [&](const toggler_targets &targets) {
		  auto &world  = state::access<adapter>::model(m_state).world;
		  auto mhandle = m_view2model.at(handle);

		  world.activators[mhandle.handle].targets.clear();
		  assert(targets.values.size() == targets.names.size());
		  for (unsigned int i = 0; i < targets.values.size(); ++i) {
			  if (targets.values[i]) {
				  world.activators[mhandle.handle].targets.push_back(m_view2model.at(m_name2view.at(targets.names[i])).handle);
			  }
		  }
	  },

	  [](const auto &) {
		  utils::log::error("adapter.adapter.activator.unsupported_edit"); // TODO add to lang files
	  }};

	for (const auto &edit : edits) {
		std::visit(visitor, edit.second);
	}
}

void adapter::adapter::edit_mob_entity(view_handle vhandle, const entity_edit &edits) {
	const std::string_view hp_str   = utils::gui_text_for("adapter.properties.mob.hp");
	const std::string_view dir_str  = utils::gui_text_for("adapter.properties.mob.direction");
	const std::string_view bhvr_str = utils::gui_text_for("adapter.properties.mob.behaviour");

	auto &world         = state::access<adapter>::model(m_state).world;
	const auto &mhandle = m_view2model.find(vhandle);

	if (mhandle == m_view2model.end()) {
		utils::log::error("adapter.unknown_view_entity", "view_handle"_a = vhandle.handle, "is_mob"_a = vhandle.is_mob);
		return;
	}

	for (const auto &edit : edits) {
		const auto &str = edit.first;

		auto visitor = [&](const auto &value) {
			using T = std::remove_cv_t<std::remove_reference_t<decltype(value)>>;

			if (str == hp_str) {
				if constexpr (!std::is_same_v<T, std::uint8_t>) { // FIXME use something else other than hard coded std::uint8_t here
					utils::log::error("adapter.properties.bad_property");
					return;
				}
				else {
					if (!world.components.health[mhandle->second.handle]) {
						utils::log::error("oh no"); // FIXME log
						return;
					}
					world.components.health[mhandle->second.handle]->points = value;
				}
			}
			else if (str == dir_str) {
				if constexpr (!std::is_same_v<T, angle>) {
					utils::log::error("adapter.properties.bad_property");
					return;
				}
				else {
					if (!world.components.hitbox[mhandle->second.handle]) {
						utils::log::error("oh no"); // FIXME log
						return;
					}
					world.components.hitbox[mhandle->second.handle]->rad = value.val;
					state::access<adapter>::view(m_state).rotate_entity(vhandle, view::facing_direction::from_angle(value.val));
				}
			}
			else if (str == bhvr_str) {
				if constexpr (!std::is_same_v<T, behaviour>) {
					utils::log::error("adapter.properties.bad_property");
					return;
				}
				else {
					world.components.metadata[mhandle->second.handle].kind = adapter_behaviour2nnj_ett_kind(value.val);
					state::access<adapter>::view(m_state).set_mob_kind(vhandle, value);
				}
			}
			else {

				utils::log::error("oh no"); // FIXME log
			}
		};

		std::visit(visitor, edit.second);
	}
}

bool adapter::adapter::can_be_toggled(view_handle handle) {
	if (handle == m_target_handle) {
		return false;
	}

	if (handle.is_mob) {
		return false;
	}

	// Retrieving model's handle
	auto it = m_view2model.find(handle);
	if (it == m_view2model.end()) {
		utils::log::warn("adapter.unknown_view_entity", "view_handle"_a = handle.handle, "is_mob"_a = handle.is_mob);
		return false;
	}


	switch (it->second.type) {
		case model_handle::ACTIVATOR:
		case model_handle::ACTIONABLE:
			return true;
		case model_handle::ENTITY:
			utils::log::warn("adapter.non_coherent_entity", "handle"_a = it->second.handle);
			return false;
	}

	return false;
}
void adapter::adapter::toggle(view_handle handle) {

	if (handle == m_target_handle || handle.is_mob) {
		utils::log::warn("adapter.non_coherent_entity", "handle"_a = handle.handle);
		return;
	}

	// Retrieving model's handle
	auto it = m_view2model.find(handle);
	if (it == m_view2model.end()) {
		utils::log::warn("adapter.unknown_view_entity", "view_handle"_a = handle.handle, "is_mob"_a = handle.is_mob);
		return;
	}

	auto& world = state::access<adapter>::model(m_state).world;

	switch (it->second.type) {
		case model_handle::ACTIVATOR:
			world.fire_activator(*this, it->second.handle, model::event_reason::NONE);
			break;
		case model_handle::ACTIONABLE:
			world.fire_actionable(*this, it->second.handle);
			break;
		case model_handle::ENTITY:
			break;
	}
}
