#include <spdlog/spdlog.h>

#include "bot/bot_api.hpp"
#include "model/components.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"

namespace bot {

void NINJACLOWN_CALLCONV ffi::log(bot::nnj_log_level level, const char *text) {
	switch (level) {
		case bot::LL_TRACE:
			spdlog::trace("[BOT] {}", text);
			break;
		case bot::LL_DEBUG:
			spdlog::debug("[BOT] {}", text);
			break;
		case bot::LL_INFO:
			spdlog::info("[BOT] {}", text);
			break;
		case bot::LL_WARN:
			spdlog::warn("[BOT] {}", text);
			break;
		case bot::LL_ERROR:
			spdlog::error("[BOT] {}", text);
			break;
		case bot::LL_CRITICAL:
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

void NINJACLOWN_CALLCONV ffi::map_scan(void *ninja_data, bot::nnj_cell *map_view) {
	model::world *world = get_world(ninja_data);
	model::grid_t &grid = world->grid;
	for (const auto &cell : grid.subgrid({0, 0}, {static_cast<utils::ssize_t>(grid.width()), static_cast<utils::ssize_t>(grid.height())})) {
		map_view->type = static_cast<bot::nnj_cell_type>(cell.type);
		if (cell.interaction_handle) {
			map_view->interaction = static_cast<bot::nnj_interaction_kind>(world->interactions[*cell.interaction_handle].kind);
		}
		++map_view; // NOLINT
	}
}

size_t NINJACLOWN_CALLCONV ffi::map_update(void *ninja_data, bot::nnj_cell *map_view, bot::nnj_cell_pos *changed_cells,
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

		const auto &model_cell  = grid[changed.x][changed.y];
		bot::nnj_cell &bot_cell = map_view[changed.x + changed.y * grid.width()]; // NOLINT

		bot_cell.type = static_cast<bot::nnj_cell_type>(model_cell.type);
		if (model_cell.interaction_handle) {
			bot_cell.interaction = static_cast<bot::nnj_interaction_kind>(world->interactions[*model_cell.interaction_handle].kind);
		}

		++changed_count;
	}

	return changed_count;
}

size_t NINJACLOWN_CALLCONV ffi::max_entities() {
	return model::cst::max_entities;
}

void NINJACLOWN_CALLCONV ffi::entities_scan(void *ninja_data, bot::nnj_entity *entities) {
	model::world *world = get_world(ninja_data);

	for (size_t i = 0; i < model::cst::max_entities; ++i) {
		entities[i].kind = world->components.metadata[i].kind;
		if (entities[i].kind != bot::nnj_entity_kind::EK_NOT_AN_ENTITY) {
			if (world->components.hitbox[i]) {
				auto &hitbox       = world->components.hitbox[i];
				entities[i].x      = hitbox->center.x;
				entities[i].y      = hitbox->center.y;
				entities[i].angle  = hitbox->rad;
				entities[i].handle = i;
			}
			else {
				// bot can't work with entity with no hitbox
				entities[i].kind = bot::nnj_entity_kind::EK_NOT_AN_ENTITY;
			}
		}
	}
}

size_t NINJACLOWN_CALLCONV ffi::entities_update(void *ninja_data, bot::nnj_entity *entities) {
	adapter::adapter *adapter = get_adapter(ninja_data);
	model::world *world       = get_world(ninja_data);

	for (size_t changed : adapter->entities_changed_since_last_update()) {
		entities[changed].kind = world->components.metadata[changed].kind;
		if (entities[changed].kind != bot::nnj_entity_kind::EK_NOT_AN_ENTITY) {
			if (world->components.hitbox[changed]) {
				auto &hitbox             = world->components.hitbox[changed];
				entities[changed].x      = hitbox->center.x;
				entities[changed].y      = hitbox->center.y;
				entities[changed].angle  = hitbox->rad;
				entities[changed].handle = changed;
			}
			else {
				// bot can't work with entity with no hitbox
				entities[changed].kind = bot::nnj_entity_kind::EK_NOT_AN_ENTITY;
			}
		}
	}

	return adapter->entities_changed_since_last_update().size();
}

void NINJACLOWN_CALLCONV ffi::commit_decision(void *ninja_data, bot::nnj_decision_commit *commits, size_t num_commits) {
	model::world *world = get_world(ninja_data);
	for (size_t i = 0; i < num_commits; ++i) {
		if (world->components.metadata[commits[i].target_handle].kind == bot::EK_DLL) {
			world->components.decision[commits[i].target_handle] = commits[i].decision;
		}
	}
}

model::model *ffi::get_model(void *ninja_data) {
	return &state::access<bot::ffi>::model(*reinterpret_cast<state::holder *>(ninja_data)); // NOLINT
}

model::world *ffi::get_world(void *ninja_data) {
	return &get_model(ninja_data)->world;
}

adapter::adapter *ffi::get_adapter(void *ninja_data) {
	return &state::access<bot::ffi>::adapter(*reinterpret_cast<state::holder *>(ninja_data)); // NOLINT
}

} // namespace bot
