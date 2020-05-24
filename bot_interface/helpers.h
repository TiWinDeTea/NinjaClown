#ifndef NINJACLOWN_BOT_INTERFACE_HELPERS_H
#define NINJACLOWN_BOT_INTERFACE_HELPERS_H

#include <stdlib.h>

#include "bot.h"

extern struct bot_api BOT;
extern struct cell *MAP;
extern size_t MAP_WIDTH;
extern size_t MAP_HEIGHT;
extern struct entity *ENTITIES;
extern size_t MAX_ENTITIES;
extern void(NINJACLOWN_CALLCONV *USER_START_LEVEL)();
extern void(NINJACLOWN_CALLCONV *USER_END_LEVEL)();

#define ninja_log(text)         BOT.log(BOT.ninja_descriptor, text)
#define ninja_map_scan()        BOT.map_scan(BOT.ninja_descriptor, MAP)
#define ninja_map_update()      BOT.map_update(BOT.ninja_descriptor, MAP)
#define ninja_entities_update() BOT.entities_update(BOT.ninja_descriptor, ENTITIES)
#define ninja_get_angle()       BOT.get_angle(BOT.ninja_descriptor)
#define ninja_get_x()           BOT.get_x_position(BOT.ninja_descriptor)
#define ninja_get_y()           BOT.get_y_position(BOT.ninja_descriptor)
#define ninja_turn_left()       BOT.turn_left(BOT.ninja_descriptor)
#define ninja_turn_right()      BOT.turn_right(BOT.ninja_descriptor)
#define ninja_move_forward()    BOT.move_forward(BOT.ninja_descriptor)
#define ninja_move_backward()   BOT.move_backward(BOT.ninja_descriptor)
#define ninja_activate_button() BOT.activate_button(BOT.ninja_descriptor)
struct cell *ninja_get_cell(size_t column, size_t line);
struct entity *ninja_get_entity(size_t handle);

#endif //NINJACLOWN_BOT_INTERFACE_HELPERS_H
