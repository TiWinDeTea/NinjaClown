#include "helpers.h"
#include "bot.h"

struct bot_api BOT;
struct nnj_cell *MAP;
size_t MAP_WIDTH;
size_t MAP_HEIGHT;
struct nnj_entity *ENTITIES;
size_t MAX_ENTITIES;
void(NINJACLOWN_CALLCONV *USER_START_LEVEL)() = NULL;
void(NINJACLOWN_CALLCONV *USER_END_LEVEL)()   = NULL;

struct nnj_cell *nnj_get_cell(size_t column, size_t line) {
	return &MAP[column + line * MAP_WIDTH];
}

struct nnj_entity *nnj_get_entity(size_t handle) {
	return &ENTITIES[handle];
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_start_level(struct bot_api api) {
	BOT = api;

	MAP_WIDTH  = api.map_width(api.ninja_descriptor);
	MAP_HEIGHT = api.map_height(api.ninja_descriptor);
	MAP        = calloc(MAP_WIDTH * MAP_HEIGHT, sizeof(struct nnj_cell));
	nnj_map_scan();

	MAX_ENTITIES = api.max_entities();
	ENTITIES     = calloc(MAX_ENTITIES, sizeof(struct nnj_entity));
	nnj_entities_scan();

	if (USER_START_LEVEL) {
		USER_START_LEVEL();
	}
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_end_level() {
	if (USER_END_LEVEL) {
		USER_END_LEVEL();
	}

	free(MAP);
	free(ENTITIES);
}
