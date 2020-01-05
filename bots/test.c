#include <stdio.h>
#include <stdlib.h>

#include "bot_interface/bot.h"

static struct bot_api BOT;

void NINJACLOWN_DLLEXPORT NINJACLOWN_STDCALL bot_init(struct bot_api api)
{
	BOT = api;
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_STDCALL bot_think()
{
	BOT.log("hello from bot");

	printf("current x=%f, y=%f, angle=%f\n", BOT.get_x_position(), BOT.get_y_position(), BOT.get_angle());

	if (rand() % 2) {
		BOT.move_forward();
		BOT.log("move forward");
	}
	else {
		if (rand() % 5 == 0) {
			BOT.activate_button();
			BOT.log("activate button");
		}
		else {
			BOT.turn_left();
			BOT.log("turn left");
		}
	}
}
