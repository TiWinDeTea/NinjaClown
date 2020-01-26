#include <stdio.h>
#include <stdlib.h>

#include "bot_interface/bot.h"

static struct bot_api BOT;
static int finished = 0;

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_init(struct bot_api api)
{
	BOT = api;
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_think()
{
	if (finished) {
		BOT.log("mission accomplished!");
		return;
	}

	if (BOT.get_angle() < 2.5f) {
		BOT.turn_left();
	} else if (BOT.get_x_position() > 7.5) {
		BOT.move_forward();
	} else {
		BOT.activate_button();
		finished = 1;
	}
}
