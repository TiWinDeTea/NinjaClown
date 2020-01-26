#ifndef NINJACLOWN_BOT_BOT_API_HPP
#define NINJACLOWN_BOT_BOT_API_HPP

#include <bot_interface/bot.h>

namespace bot::ffi {

void NINJACLOWN_CALLCONV log(const char *text);

bot::cell **NINJACLOWN_CALLCONV vision();

float NINJACLOWN_CALLCONV get_angle();
float NINJACLOWN_CALLCONV get_x_position();
float NINJACLOWN_CALLCONV get_y_position();

void NINJACLOWN_CALLCONV turn_right();
void NINJACLOWN_CALLCONV turn_left();
void NINJACLOWN_CALLCONV move_forward();
void NINJACLOWN_CALLCONV move_backward();
void NINJACLOWN_CALLCONV move_backward_dummy();

void NINJACLOWN_CALLCONV activate_button();

} // namespace bot::ffi

#endif //NINJACLOWN_BOT_BOT_API_HPP
