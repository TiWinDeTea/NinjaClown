#include <cmath>
#include <ninja_clown/api.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "bot/bot_api.hpp"
#include "model/components.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"
#include "utils/logging.hpp"

using fmt::literals::operator""_a;

namespace {

void fill_entity_struct(const ::model::components &components, ninja_api::nnj_entity *entity) {
	entity->kind = components.metadata[entity->handle].kind;
	if (entity->kind != ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY) {
		if (components.hitbox[entity->handle]) {
			const auto &hitbox = components.hitbox[entity->handle];
			entity->x          = hitbox->center.x;
			entity->y          = hitbox->center.y;
			entity->angle      = hitbox->rad;

			const auto &properties            = components.properties[entity->handle];
			entity->properties.move_speed     = properties.move_speed;
			entity->properties.rotation_speed = properties.rotation_speed;
			entity->properties.attack_range   = properties.attack_range;
			entity->properties.activate_range = properties.activate_range;
			entity->properties.attack_delay   = static_cast<float>(properties.attack_delay);
			entity->properties.throw_delay    = static_cast<float>(properties.throw_delay);

			if (components.state[entity->handle].preparing_action.has_value()) {
				entity->state = ninja_api::nnj_entity_state::ES_BUSY;
			}
			else {
				entity->state = ninja_api::nnj_entity_state::ES_READY;
			}
		}
		else {
			// bot can't work with entity with no hitbox
			entity->kind = ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY;
		}
	}
}

} // namespace

namespace bot {

ffi::operator ninja_api::nnj_api() noexcept {
	ninja_api::nnj_api api{};

	api.log = &ffi::log;

	api.map_width  = &ffi::map_width;
	api.map_height = &ffi::map_height;
	api.target_position = &ffi::target_position;
	api.map_scan   = &ffi::map_scan;
	api.map_update = &ffi::map_update;

	api.max_entities    = &ffi::max_entities;
	api.entities_scan   = &ffi::entities_scan;
	api.entities_update = &ffi::entities_update;

	api.commit_decisions = &ffi::commit_decisions;
	return api;
}

void NINJACLOWN_CALLCONV ffi::log(ninja_api::nnj_log_level level, const char *text) {
	switch (level) {
		case ninja_api::LL_TRACE:
			spdlog::trace("[BOT] {}", text);
			break;
		case ninja_api::LL_DEBUG:
			spdlog::debug("[BOT] {}", text);
			break;
		case ninja_api::LL_INFO:
			spdlog::info("[BOT] {}", text);
			break;
		case ninja_api::LL_WARN:
			spdlog::warn("[BOT] {}", text);
			break;
		case ninja_api::LL_ERROR:
			spdlog::error("[BOT] {}", text);
			break;
		case ninja_api::LL_CRITICAL:
			spdlog::critical("[BOT] {}", text);
			break;
		default:
			spdlog::info("[BOT] {}", text);
			break;
	}
}

size_t NINJACLOWN_CALLCONV ffi::map_width(void *ninja_data) {
	return get_world(ninja_data)->grid.width();
}

size_t NINJACLOWN_CALLCONV ffi::map_height(void *ninja_data) {
	return get_world(ninja_data)->grid.height();
}

ninja_api::nnj_cell_pos NINJACLOWN_CALLCONV ffi::target_position(void *ninja_data) {
	model::grid_point &target = get_world(ninja_data)->target_tile;
	return ninja_api::nnj_cell_pos { static_cast<size_t>(target.x), static_cast<size_t>(target.y) };
}

void NINJACLOWN_CALLCONV ffi::map_scan(void *ninja_data, ninja_api::nnj_cell *map_view) {
	model::world *world = get_world(ninja_data);
	model::grid_t &grid = world->grid;
	for (const auto &cell : grid.subgrid({0, 0}, {static_cast<utils::ssize_t>(grid.width()), static_cast<utils::ssize_t>(grid.height())})) {
		map_view->kind = static_cast<ninja_api::nnj_cell_kind>(cell.type);
		if (cell.interaction_handle) {
			map_view->interaction = static_cast<ninja_api::nnj_interaction_kind>(world->interactions[*cell.interaction_handle].kind);
		}
		++map_view; // NOLINT
	}
}

size_t NINJACLOWN_CALLCONV ffi::map_update(void *ninja_data, ninja_api::nnj_cell *map_view, ninja_api::nnj_cell_pos *changed_cells,
                                           size_t changed_size) {
	adapter::adapter *adapter = get_adapter(ninja_data);
	model::world *world       = get_world(ninja_data);
	model::grid_t &grid       = world->grid;
	size_t changed_count      = 0;

	for (const auto &changed : adapter->cells_changed_since_last_update()) {
		if (changed_count < changed_size) {
			changed_cells[changed_count].column = changed.x; // NOLINT
			changed_cells[changed_count].line   = changed.y; // NOLINT
		}

		const auto &model_cell        = grid[changed.x][changed.y];
		ninja_api::nnj_cell &bot_cell = map_view[changed.x + changed.y * grid.width()]; // NOLINT

		bot_cell.kind = static_cast<ninja_api::nnj_cell_kind>(model_cell.type);
		if (model_cell.interaction_handle) {
			bot_cell.interaction = static_cast<ninja_api::nnj_interaction_kind>(world->interactions[*model_cell.interaction_handle].kind);
		}

		++changed_count;
	}

	return changed_count;
}

size_t NINJACLOWN_CALLCONV ffi::max_entities() {
	return model::cst::max_entities;
}

void NINJACLOWN_CALLCONV ffi::entities_scan(void *ninja_data, ninja_api::nnj_entity *entities) {
	model::world *world = get_world(ninja_data);

	for (size_t i = 0; i < model::cst::max_entities; ++i) {
		entities[i].handle = i; // NOLINT
		fill_entity_struct(world->components, entities + i); // NOLINT
	}
}

size_t NINJACLOWN_CALLCONV ffi::entities_update(void *ninja_data, ninja_api::nnj_entity *entities) {
	adapter::adapter *adapter = get_adapter(ninja_data);
	model::world *world       = get_world(ninja_data);

	for (size_t changed : adapter->entities_changed_since_last_update()) {
		entities[changed].handle = changed; // NOLINT
		fill_entity_struct(world->components, entities + changed); // NOLINT
	}

	return adapter->entities_changed_since_last_update().size();
}

#define SANITIZE(x)                                                                                                                        \
	if (std::isinf((x)) || std::isnan((x))) {                                                                                              \
		utils::log::warn(*get_resources(ninja_data), "bot_api.commit.sanitize", "value"_a = (x), "parameter"_a = #x);                      \
		(x) = 0.0f;                                                                                                                        \
	}

void NINJACLOWN_CALLCONV ffi::commit_decisions(void *ninja_data, ninja_api::nnj_decision_commit const *commits, size_t num_commits) {
	model::world *world = get_world(ninja_data);
	for (size_t i = 0; i < num_commits; ++i) {
		ninja_api::nnj_decision_commit const &commit = commits[i]; // NOLINT
		if (commit.target_handle > model::cst::max_entities) {
			utils::log::warn(*get_resources(ninja_data), "bot_api.commit.invalid_handle", "handle"_a = commit.target_handle,
			                 "decision"_a = i);
		}
		else if (world->components.metadata[commit.target_handle].kind == ninja_api::EK_DLL) {
			switch (commit.decision.kind) {
				case ninja_api::DK_NONE:
					world->components.decision[commit.target_handle] = {};
					break;
				case ninja_api::DK_MOVEMENT: {
					ninja_api::nnj_movement_request req = commit.decision.movement_req; // NOLINT
					SANITIZE(req.forward_diff)
					SANITIZE(req.lateral_diff)
					SANITIZE(req.rotation)
					world->components.decision[commit.target_handle] = {req}; // NOLINT
					break;
				}
				case ninja_api::DK_ACTIVATE:
					world->components.decision[commit.target_handle] = {commit.decision.activate_req}; // NOLINT
					break;
				case ninja_api::DK_ATTACK:
					world->components.decision[commit.target_handle] = {commit.decision.attack_req}; // NOLINT
					break;
				case ninja_api::DK_THROW:
					world->components.decision[commit.target_handle] = {commit.decision.throw_req}; // NOLINT
					break;
				default:
					spdlog::warn("DLL sent invalid decision kind ({}) for decision {}", commit.decision.kind, i);
					break;
			}
		}
	}
}

model::model *ffi::get_model(void *ninja_data) {
	return &state::access<bot::ffi>::model(*reinterpret_cast<state::holder *>(ninja_data)); // NOLINT
}

utils::resource_manager *ffi::get_resources(void *ninja_data) {
	return &reinterpret_cast<state::holder *>(ninja_data)->resources(); // NOLINT
}

model::world *ffi::get_world(void *ninja_data) {
	return &get_model(ninja_data)->world;
}

adapter::adapter *ffi::get_adapter(void *ninja_data) {
	return &state::access<bot::ffi>::adapter(*reinterpret_cast<state::holder *>(ninja_data)); // NOLINT
}

} // namespace bot
