#include <spdlog/spdlog.h>

#include "bot/bot_api.hpp"
#include "model/components.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"

namespace {

void set_decision(model::world &world, model::component::decision decision) {
	world.components.decision[world.ninja_clown_handle] = {decision};
}

} // namespace

namespace bot {

void NINJACLOWN_CALLCONV ffi::log(const char *text) {
	spdlog::info("{}", text);
}

size_t NINJACLOWN_CALLCONV ffi::map_width(void *ninja_data) {
	model::world &world = get_world(ninja_data);
	return world.grid.width();
}

size_t NINJACLOWN_CALLCONV ffi::map_height(void *ninja_data) {
	model::world &world = get_world(ninja_data);
	return world.grid.height();
}

void NINJACLOWN_CALLCONV ffi::map_scan(void *ninja_data, bot::cell *map_view) {
	model::world &world = get_world(ninja_data);
	model::grid_t &grid = world.grid;
	for (const auto &cell : grid.subgrid(0, 0, grid.width(), grid.height())) {
		map_view->type = static_cast<bot::cell_type>(cell.type);
		if (cell.interaction_handle) {
			map_view->interaction = static_cast<bot::interaction_kind>(world.interactions[*cell.interaction_handle].kind);
		}
		++map_view;
	}
}

void NINJACLOWN_CALLCONV ffi::map_update(void *ninja_data, bot::cell *map_view) {
	spdlog::warn("map_update function is not implemented yet");
}

float NINJACLOWN_CALLCONV ffi::get_angle(void *ninja_data) {
	model::world &world = get_world(ninja_data);
	return world.components.hitbox[world.ninja_clown_handle]->rad;
}

float NINJACLOWN_CALLCONV ffi::get_x_position(void *ninja_data) {
	model::world &world = get_world(ninja_data);
	return world.components.hitbox[world.ninja_clown_handle]->center.x;
}

float NINJACLOWN_CALLCONV ffi::get_y_position(void *ninja_data) {
	model::world &world = get_world(ninja_data);
	return world.components.hitbox[world.ninja_clown_handle]->center.y;
}

void NINJACLOWN_CALLCONV ffi::turn_right(void *ninja_data) {
	set_decision(get_world(ninja_data), model::component::decision::TURN_RIGHT);
}

void NINJACLOWN_CALLCONV ffi::turn_left(void *ninja_data) {
	set_decision(get_world(ninja_data), model::component::decision::TURN_LEFT);
}

void NINJACLOWN_CALLCONV ffi::move_forward(void *ninja_data) {
	set_decision(get_world(ninja_data), model::component::decision::MOVE_FORWARD);
}

void NINJACLOWN_CALLCONV ffi::move_backward(void *ninja_data) {
	set_decision(get_world(ninja_data), model::component::decision::MOVE_BACKWARD);
}

void NINJACLOWN_CALLCONV ffi::move_backward_dummy([[maybe_unused]] void *model) {
	spdlog::warn("called 'move_backward_dummy', but the bot does not know how to do that!");
}

void NINJACLOWN_CALLCONV ffi::activate_button(void *ninja_data) {
	set_decision(get_world(ninja_data), model::component::decision::ACTIVATE_BUTTON);
}

model::model &ffi::get_model(void *ninja_data) {
	return state::access<bot::ffi>::model(*reinterpret_cast<state::holder *>(ninja_data));
}

model::world &ffi::get_world(void *ninja_data) {
	return get_model(ninja_data).world;
}

adapter::adapter &ffi::get_adapter(void *ninja_data) {
	return state::access<bot::ffi>::adapter(*reinterpret_cast<state::holder *>(ninja_data));
}

} // namespace bot
