#include <spdlog/spdlog.h>

#include "bot/bot_api.hpp"
#include "model/components.hpp"
#include "model/model.hpp"

namespace {

size_t ninja_clown_handle(model::model *model) {
	return model->world.ninja_clown_handle;
}

void set_decision(void *model, model::component::decision decision) {
	auto data                                                 = reinterpret_cast<model::model *>(model);
	data->world.components.decision[ninja_clown_handle(data)] = {decision};
}

} // namespace

namespace bot::ffi {

void NINJACLOWN_CALLCONV log(const char *text) {
	spdlog::info("{}", text);
}

size_t NINJACLOWN_CALLCONV map_width(void *model) {
    auto data = reinterpret_cast<model::model *>(model);
    return data->world.grid.width();
}

size_t NINJACLOWN_CALLCONV map_height(void *model) {
    auto data = reinterpret_cast<model::model *>(model);
    return data->world.grid.height();
}

void NINJACLOWN_CALLCONV map_scan(void *model, bot::cell *map_view) {
    auto data = reinterpret_cast<model::model *>(model);
    model::grid_t& grid = data->world.grid;
    for (const auto& cell : grid.subgrid(0, 0, grid.width(), grid.height())) {
        map_view->type = static_cast<bot::cell_type>(cell.type);
        if (cell.interaction_handle) {
            map_view->interaction = static_cast<bot::interaction_kind>(data->world.interactions[*cell.interaction_handle].kind);
        }
        ++map_view;
    }
}

void NINJACLOWN_CALLCONV map_update(void *model, bot::cell *map_view) {
    spdlog::warn("map_update function is not implemented yet");
}

float NINJACLOWN_CALLCONV get_angle(void *model) {
	auto data = reinterpret_cast<model::model *>(model);
	return data->world.components.hitbox[ninja_clown_handle(data)]->rad;
}

float NINJACLOWN_CALLCONV get_x_position(void *model) {
	auto data = reinterpret_cast<model::model *>(model);
	return data->world.components.hitbox[ninja_clown_handle(data)]->center.x;
}

float NINJACLOWN_CALLCONV get_y_position(void *model) {
	auto data = reinterpret_cast<model::model *>(model);
	return data->world.components.hitbox[ninja_clown_handle(data)]->center.y;
}

void NINJACLOWN_CALLCONV turn_right(void *model) {
	set_decision(model, model::component::decision::TURN_RIGHT);
}

void NINJACLOWN_CALLCONV turn_left(void *model) {
	set_decision(model, model::component::decision::TURN_LEFT);
}

void NINJACLOWN_CALLCONV move_forward(void *model) {
	set_decision(model, model::component::decision::MOVE_FORWARD);
}

void NINJACLOWN_CALLCONV move_backward(void *model) {
	set_decision(model, model::component::decision::MOVE_BACKWARD);
}

void NINJACLOWN_CALLCONV move_backward_dummy([[maybe_unused]] void *model) {
	spdlog::warn("called 'move_backward_dummy', but the bot does not know how to do that!");
}

void NINJACLOWN_CALLCONV activate_button(void *model) {
	set_decision(model, model::component::decision::ACTIVATE_BUTTON);
}

} // namespace bot::ffi
