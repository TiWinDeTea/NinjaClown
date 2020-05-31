#include <spdlog/spdlog.h>

#include "bot/bot_api.hpp"
#include "model/components.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"

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
		}
		else {
			// bot can't work with entity with no hitbox
			entity->kind = ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY;
		}
	}
}

} // namespace

namespace bot {

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

void NINJACLOWN_CALLCONV ffi::commit_decisions(void *ninja_data, ninja_api::nnj_decision_commit const *commits, size_t num_commits) {
	model::world *world = get_world(ninja_data);
	for (size_t i = 0; i < num_commits; ++i) {
		if (commits[i].target_handle > model::cst::max_entities) { // NOLINT
			spdlog::warn("bot returned invalid target handle ({}) for decision {}", commits[i].target_handle, i);
		}
		else {
			if (world->components.metadata[commits[i].target_handle].kind == ninja_api::EK_DLL) { // NOLINT
				world->components.decision[commits[i].target_handle] = commits[i].decision; // NOLINT
			}
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
