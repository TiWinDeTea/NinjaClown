#ifndef NINJACLOWN_BOT_INTERFACE_HELPERS_H
#define NINJACLOWN_BOT_INTERFACE_HELPERS_H

#include "bot.h"

extern struct bot_api BOT;

#define ninja_log(text) BOT.log(text)
#define ninja_vision() BOT.vision(BOT.ninja_descriptor)
#define ninja_get_angle() BOT.get_angle(BOT.ninja_descriptor)
#define ninja_get_x() BOT.get_x_position(BOT.ninja_descriptor)
#define ninja_get_y() BOT.get_y_position(BOT.ninja_descriptor)
#define ninja_turn_left() BOT.turn_left(BOT.ninja_descriptor)
#define ninja_turn_right() BOT.turn_right(BOT.ninja_descriptor)
#define ninja_move_forward() BOT.move_forward(BOT.ninja_descriptor)
#define ninja_move_backward() BOT.move_backward(BOT.ninja_descriptor)
#define ninja_activate_button() BOT.activate_button(BOT.ninja_descriptor)

#ifdef NINJAHELPER_IMPLEMENT
struct bot_api BOT;

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_init(struct bot_api api)
{
	BOT = api;
}
#endif

#endif //NINJACLOWN_BOT_INTERFACE_HELPERS_H
