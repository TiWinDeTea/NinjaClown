#include <bot_interface/bot.h>
#include <bot_interface/helpers.h>

static _Bool finished;
static int action_choice;

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV on_start() {
	finished      = 0;
	action_choice = 0;
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_init() {
	USER_START_LEVEL = on_start;
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_destroy() {
	ninja_log("No... no... NO! DON'T DESTROY M");
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_think() {
	ninja_map_update();
	ninja_entities_update();

	if (finished) {
		action_choice = action_choice + 1;
		if (action_choice >= 3) {
			action_choice = 0;
			ninja_turn_right();
		}
		else {
			ninja_move_forward();
		}

		return;
	}

	if (ninja_get_angle() < 2.8f) {
		ninja_turn_left();
	}
	else if (ninja_get_x() > 7.5) {
		ninja_move_forward();
	}
	else {
		ninja_activate_button();
		finished = 1;
	}
}
