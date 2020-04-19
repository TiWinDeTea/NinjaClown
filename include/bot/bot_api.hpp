#ifndef NINJACLOWN_BOT_BOT_API_HPP
#define NINJACLOWN_BOT_BOT_API_HPP

#include <bot_interface/bot.h>

namespace bot::ffi {

void NINJACLOWN_CALLCONV log(const char *text);

size_t NINJACLOWN_CALLCONV map_width(void* ninja_data);
size_t NINJACLOWN_CALLCONV map_height(void* ninja_data);
void NINJACLOWN_CALLCONV map_scan(void* ninja_data, bot::cell *map_view);
void NINJACLOWN_CALLCONV map_update(void* ninja_data, bot::cell *map_view);

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
