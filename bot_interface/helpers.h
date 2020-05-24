#ifndef NINJACLOWN_BOT_INTERFACE_HELPERS_H
#define NINJACLOWN_BOT_INTERFACE_HELPERS_H

#include <stdlib.h>

#include "bot.h"

extern struct bot_api BOT;
extern struct nnj_cell *MAP;
extern size_t MAP_WIDTH;
extern size_t MAP_HEIGHT;
extern struct nnj_entity *ENTITIES;
extern size_t MAX_ENTITIES;
extern void(NINJACLOWN_CALLCONV *USER_START_LEVEL)();
extern void(NINJACLOWN_CALLCONV *USER_END_LEVEL)();

struct nnj_cell *nnj_get_cell(size_t column, size_t line);
struct nnj_entity *nnj_get_entity(size_t handle);

#define nnj_log(log_level, text)                  BOT.log(log_level, text)
#define nnj_map_scan()                            BOT.map_scan(BOT.ninja_descriptor, MAP)
#define nnj_map_update()                          BOT.map_update(BOT.ninja_descriptor, MAP, NULL, 0)
#define nnj_entities_scan()                       BOT.entities_update(BOT.ninja_descriptor, ENTITIES)
#define nnj_entities_update()                     BOT.entities_update(BOT.ninja_descriptor, ENTITIES)
#define nnj_commit_decision(commits, num_commits) BOT.commit_decision(BOT.ninja_descriptor, commits, num_commits);

#endif //NINJACLOWN_BOT_INTERFACE_HELPERS_H
