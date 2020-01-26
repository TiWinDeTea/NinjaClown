#include <spdlog/spdlog.h>

#include "bot/bot_api.hpp"
#include "model/components.hpp"
#include "program_state.hpp"

namespace {

size_t ninja_clown_handle() {
	return program_state::global->world.ninja_clown_handle;
}

void set_decision(model::component::decision decision) {
	program_state::global->world.components.decision[ninja_clown_handle()] = {decision};
}

} // namespace

namespace bot::ffi {

void NINJACLOWN_CALLCONV log(const char *text) {
	spdlog::info("{}", text);
}

bot::cell ** NINJACLOWN_CALLCONV vision() {
	spdlog::warn("bot attempted to use vision function, but this isn't supported yet");
	return nullptr;
}

float NINJACLOWN_CALLCONV get_angle() {
	return program_state::global->world.components.angle[ninja_clown_handle()]->rad;
}

float NINJACLOWN_CALLCONV get_x_position() {
	return program_state::global->world.components.hitbox[ninja_clown_handle()]->center_x();
}

float NINJACLOWN_CALLCONV get_y_position() {
	return program_state::global->world.components.hitbox[ninja_clown_handle()]->center_y();
}

void NINJACLOWN_CALLCONV turn_right() {
	set_decision(model::component::decision::TURN_RIGHT);
}

void NINJACLOWN_CALLCONV turn_left() {
	set_decision(model::component::decision::TURN_LEFT);
}

void NINJACLOWN_CALLCONV move_forward() {
	set_decision(model::component::decision::MOVE_FORWARD);
}

void NINJACLOWN_CALLCONV move_backward() {
	set_decision(model::component::decision::MOVE_BACKWARD);
}

void NINJACLOWN_CALLCONV move_backward_dummy() {
	spdlog::warn("called 'move_backward_dummy', but the bot does not know how to do that!");
}

void NINJACLOWN_CALLCONV activate_button() {
	set_decision(model::component::decision::ACTIVATE_BUTTON);
}

} // namespace bot::ffi
