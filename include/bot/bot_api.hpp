#ifndef NINJACLOWN_BOT_BOT_API_HPP
#define NINJACLOWN_BOT_BOT_API_HPP

#include <bot_interface/bot.h>

namespace bot::ffi {

void NINJACLOWN_STDCALL log(const char *text);

bot::cell **NINJACLOWN_STDCALL vision();

float NINJACLOWN_STDCALL get_angle();
float NINJACLOWN_STDCALL get_x_position();
float NINJACLOWN_STDCALL get_y_position();

void NINJACLOWN_STDCALL turn_right();
void NINJACLOWN_STDCALL turn_left();
void NINJACLOWN_STDCALL move_forward();
void NINJACLOWN_STDCALL move_backward();
void NINJACLOWN_STDCALL move_backward_dummy();

void NINJACLOWN_STDCALL activate_button();

} // namespace bot::ffi

#endif //NINJACLOWN_BOT_BOT_API_HPP
