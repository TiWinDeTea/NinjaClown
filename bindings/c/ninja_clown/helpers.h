#ifndef NINJACLOWN_NINJA_CLOWN_BOT_H
#define NINJACLOWN_NINJA_CLOWN_BOT_H

#include <ninja_clown/api.h>
#include <stdlib.h>

extern struct nnj_api BOT;
extern struct nnj_cell *MAP;
extern size_t MAP_WIDTH;
extern size_t MAP_HEIGHT;
extern struct nnj_entity *ENTITIES;
extern size_t MAX_ENTITIES;

extern void (*NNJ_START_LEVEL_CALLBACK)();
extern void (*NNJ_END_LEVEL_CALLBACK)();
extern void (*NNJ_INIT_CALLBACK)();
extern void (*NNJ_DESTROY_CALLBACK)();
extern void (*NNJ_THINK_CALLBACK)();

#define nnj_log(log_level, text)                   BOT.log(log_level, text)
#define nnj_map_scan()                             BOT.map_scan(BOT.ninja_descriptor, MAP)
#define nnj_map_update()                           BOT.map_update(BOT.ninja_descriptor, MAP, NULL, 0)
#define nnj_entities_scan()                        BOT.entities_scan(BOT.ninja_descriptor, ENTITIES)
#define nnj_entities_update()                      BOT.entities_update(BOT.ninja_descriptor, ENTITIES)
#define nnj_commit_decisions(commits, num_commits) BOT.commit_decisions(BOT.ninja_descriptor, commits, num_commits);

struct nnj_cell *nnj_get_cell(size_t column, size_t line);
struct nnj_entity *nnj_get_entity(size_t handle);
struct nnj_decision nnj_build_decision_none();
struct nnj_decision nnj_build_decision_movement(float rotation, float forward_diff, float lateral_diff);
struct nnj_decision nnj_build_decision_attack(size_t target_handle);
struct nnj_decision nnj_build_decision_activate(size_t column, size_t line);
struct nnj_decision nnj_build_decision_throw();

#ifdef NINJACLOWN_HELPERS_IMPLEMENT

struct nnj_api BOT;
struct nnj_cell *MAP;
size_t MAP_WIDTH;
size_t MAP_HEIGHT;
struct nnj_entity *ENTITIES;
size_t MAX_ENTITIES;

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_init() {
	if (NNJ_INIT_CALLBACK) {
		NNJ_INIT_CALLBACK();
	}
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_start_level(struct nnj_api api) {
	BOT = api;

	MAP_WIDTH  = api.map_width(api.ninja_descriptor);
	MAP_HEIGHT = api.map_height(api.ninja_descriptor);
	MAP        = calloc(MAP_WIDTH * MAP_HEIGHT, sizeof(struct nnj_cell));
	nnj_map_scan();

	MAX_ENTITIES = api.max_entities();
	ENTITIES     = calloc(MAX_ENTITIES, sizeof(struct nnj_entity));
	nnj_entities_scan();

	if (NNJ_START_LEVEL_CALLBACK) {
		NNJ_START_LEVEL_CALLBACK();
	}
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_think() {
	NNJ_THINK_CALLBACK();
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_end_level() {
	if (NNJ_END_LEVEL_CALLBACK) {
		NNJ_END_LEVEL_CALLBACK();
	}

	free(MAP);
	free(ENTITIES);
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_destroy() {
	if (NNJ_DESTROY_CALLBACK) {
		NNJ_DESTROY_CALLBACK();
	}
}

struct nnj_decision nnj_build_decision_none() {
	struct nnj_decision decision;
	decision.kind = DK_NONE;
	return decision;
}

struct nnj_decision nnj_build_decision_movement(float rotation, float forward_diff, float lateral_diff) {
	struct nnj_decision decision;
	decision.kind = DK_MOVEMENT;
	struct nnj_movement_request move_req;
	move_req.rotation     = rotation;
	move_req.forward_diff = forward_diff;
	move_req.lateral_diff = lateral_diff;
	decision.movement_req = move_req;
	return decision;
}

struct nnj_decision nnj_build_decision_attack(size_t target_handle) {
	struct nnj_decision decision;
	decision.kind = DK_ATTACK;
	struct nnj_attack_request attack_req;
	attack_req.target_handle = target_handle;
	decision.attack_req      = attack_req;
	return decision;
}

struct nnj_decision nnj_build_decision_activate(size_t column, size_t line) {
	struct nnj_decision decision;
	decision.kind = DK_ACTIVATE;
	struct nnj_activate_request activate_req;
	activate_req.column   = column;
	activate_req.line     = line;
	decision.activate_req = activate_req;
	return decision;
}

struct nnj_decision nnj_build_decision_throw() {
	struct nnj_decision decision;
	decision.kind = DK_THROW;
	return decision;
}

#endif //NINJACLOWN_BOT_IMPLEMENT

#endif //NINJACLOWN_NINJA_CLOWN_BOT_H
