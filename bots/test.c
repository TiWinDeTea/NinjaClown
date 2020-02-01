#include <stdio.h>
#include <stdlib.h>

#include "bot_interface/bot.h"

#define NINJAHELPER_IMPLEMENT
#include "bot_interface/helpers.h"

static _Bool finished = 0;

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_think()
{
	if (finished) {
		ninja_log("mission accomplished!");
		return;
	}

	if (ninja_get_angle() < 2.5f) {
		ninja_turn_left();
	} else if (ninja_get_x() > 7.5) {
		ninja_move_forward();
	} else {
		ninja_activate_button();
		finished = 1;
	}
}
