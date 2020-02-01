#ifndef NINJACLOWN_BOT_BOT_API_HPP
#define NINJACLOWN_BOT_BOT_API_HPP

#include <bot_interface/bot.h>

namespace bot::ffi {

void NINJACLOWN_CALLCONV log(const char *text);

bot::cell **NINJACLOWN_CALLCONV vision(void* ninja_data);

float NINJACLOWN_CALLCONV get_angle(void* ninja_data);
float NINJACLOWN_CALLCONV get_x_position(void* ninja_data);
float NINJACLOWN_CALLCONV get_y_position(void* ninja_data);

void NINJACLOWN_CALLCONV turn_right(void* ninja_data);
void NINJACLOWN_CALLCONV turn_left(void* ninja_data);
void NINJACLOWN_CALLCONV move_forward(void* ninja_data);
void NINJACLOWN_CALLCONV move_backward(void* ninja_data);
void NINJACLOWN_CALLCONV move_backward_dummy(void* ninja_data);

void NINJACLOWN_CALLCONV activate_button(void* ninja_data);

} // namespace bot::ffi

#endif //NINJACLOWN_BOT_BOT_API_HPP
