#ifndef NINJACLOWN_BOT_INTERFACE_HELPERS_H
#define NINJACLOWN_BOT_INTERFACE_HELPERS_H

#include <stdlib.h>

#include "bot.h"

extern struct bot_api BOT;
extern struct cell *MAP;
extern size_t MAP_WIDTH;
extern size_t MAP_HEIGHT;
extern void(NINJACLOWN_CALLCONV *USER_START_LEVEL)();
extern void(NINJACLOWN_CALLCONV *USER_DESTROY)();

#define ninja_log(text)         BOT.log(text)
#define ninja_map_scan()        BOT.map_scan(BOT.ninja_descriptor, MAP)
#define ninja_map_update()      BOT.map_update(BOT.ninja_descriptor, MAP)
#define ninja_get_angle()       BOT.get_angle(BOT.ninja_descriptor)
#define ninja_get_x()           BOT.get_x_position(BOT.ninja_descriptor)
#define ninja_get_y()           BOT.get_y_position(BOT.ninja_descriptor)
#define ninja_turn_left()       BOT.turn_left(BOT.ninja_descriptor)
#define ninja_turn_right()      BOT.turn_right(BOT.ninja_descriptor)
#define ninja_move_forward()    BOT.move_forward(BOT.ninja_descriptor)
#define ninja_move_backward()   BOT.move_backward(BOT.ninja_descriptor)
#define ninja_activate_button() BOT.activate_button(BOT.ninja_descriptor)

struct cell *ninja_get_cell(size_t column, size_t line) {
	return &MAP[column + line * MAP_WIDTH];
}

#ifdef NINJAHELPER_IMPLEMENT
struct bot_api BOT;
struct cell *MAP;
size_t MAP_WIDTH;
size_t MAP_HEIGHT;
void(NINJACLOWN_CALLCONV *USER_START_LEVEL)() = NULL;
void(NINJACLOWN_CALLCONV *USER_DESTROY)()     = NULL;

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_start_level(struct bot_api api) {
	BOT = api;

	if (MAP) {
		free(MAP);
	}

	MAP_WIDTH  = api.map_width(api.ninja_descriptor);
	MAP_HEIGHT = api.map_height(api.ninja_descriptor);
	MAP        = calloc(MAP_WIDTH * MAP_HEIGHT, sizeof(struct cell));
	ninja_map_scan();

	if (USER_START_LEVEL) {
		USER_START_LEVEL();
	}
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_destroy() {
	if (USER_DESTROY) {
		USER_DESTROY();
	}

	free(MAP);
}

// TODO: callback for end level?
#endif

#endif //NINJACLOWN_BOT_INTERFACE_HELPERS_H
