#include <algorithm>
#include <spdlog/spdlog.h>

#include "bot/bot_api.hpp"
#include "model/components.hpp"
#include "program_state.hpp"
#include "utils/optional.hpp"

namespace {

size_t ninja_clown_handle()
{
	return program_state::global->world.ninja_clown_handle;
}

void set_decision(model::component::decision decision)
{
	program_state::global->world.components.decision[ninja_clown_handle()] = {decision};
}

} // namespace

namespace bot::ffi {

void log(const char *text)
{
	spdlog::info("{}", text);
}

bot::cell **vision()
{
	spdlog::warn("bot attempted to use vision function, but this isn't supported yet");
	return nullptr;
}

float get_angle()
{
	return program_state::global->world.components.angle[ninja_clown_handle()]->rad;
}

float get_x_position()
{
	return program_state::global->world.components.hitbox[ninja_clown_handle()]->center_x();
}

float get_y_position()
{
	return program_state::global->world.components.hitbox[ninja_clown_handle()]->center_y();
}

void turn_right()
{
	set_decision(model::component::decision::TURN_RIGHT);
}

void turn_left()
{
	set_decision(model::component::decision::TURN_LEFT);
}

void move_forward()
{
	set_decision(model::component::decision::MOVE_FORWARD);
}

void move_backward()
{
	set_decision(model::component::decision::MOVE_BACKWARD);
}

void move_backward_dummy()
{
	spdlog::warn("called 'move_backward_dummy', but the bot does not know how to do that!");
}

void activate_button()
{
	set_decision(model::component::decision::ACTIVATE_BUTTON);
}

} // namespace bot::ffi
