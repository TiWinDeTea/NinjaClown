#ifndef NINJACLOWN_BOT_INTERFACE_BOT_H
#define NINJACLOWN_BOT_INTERFACE_BOT_H

#ifdef OS_WINDOWS
#	define NINJACLOWN_DLLEXPORT __declspec(dllexport)
#	define NINJACLOWN_CALLCONV  __cdecl
#else
#	define NINJACLOWN_DLLEXPORT
#	define NINJACLOWN_CALLCONV __attribute((sysv_abi))
#endif

#ifdef __cplusplus
#	include <cstddef>
namespace bot {
extern "C" {
#else
#	include <stddef.h>
#endif

enum nnj_log_level {
	LL_TRACE    = 0,
	LL_DEBUG    = 1,
	LL_INFO     = 2,
	LL_WARN     = 3,
	LL_ERROR    = 4,
	LL_CRITICAL = 5,
};

enum nnj_cell_type {
	CT_UNKNOWN = 0,
	CT_CHASM   = 1,
	CT_GROUND  = 2,
	CT_WALL    = 3,
};

enum nnj_interaction_kind {
	IK_NO_INTERACTION = 0, // not an interactive cell
	IK_LIGHT_MANUAL   = 1, // character or thrown item can interact
	IK_HEAVY_MANUAL   = 2, // only a character can interact
	IK_LIGHT_MIDAIR   = 3, // character or thrown item in the cell cause interaction
	IK_HEAVY_MIDAIR   = 4, // only character in the cell cause interaction
	IK_WALK_ON_GROUND = 5, // only non-floating character in the cell cause interaction
};

struct nnj_cell {
	enum nnj_cell_type type;
	enum nnj_interaction_kind interaction;
};

struct nnj_cell_pos {
	size_t column;
	size_t line;
};

enum nnj_entity_kind {
	EK_NOT_AN_ENTITY = 0, // unused entity slot
	EK_HARMLESS      = 1, // entity is harmless
	EK_PATROL        = 2, // patrolling entity
	EK_AGGRESSIVE    = 3, // rush to the player
	EK_PROJECTILE    = 4, // follow some trajectory
	EK_DLL           = 5, // controlled by dll
};

struct nnj_entity {
	enum nnj_entity_kind kind;
	float x, y;
	float angle;
	size_t handle;
	// TODO: expose properties
};

enum nnj_decision_kind {
	DK_NONE     = 0,
	DK_MOVEMENT = 1,
	DK_ACTIVATE = 2,
	DK_ATTACK   = 3,
	DK_THROW    = 4,
};

struct nnj_movement_request {
	float rotation; // radians
	float forward_diff;
	float lateral_diff;
};

struct nnj_activate_request {
	size_t column;
	size_t line;
};

struct nnj_attack_request {
	size_t target_handle;
};

struct nnj_decision {
	enum nnj_decision_kind kind;
	union {
		struct nnj_movement_request movement;
		struct nnj_activate_request activate;
		struct nnj_attack_request attack;
	};
};

struct nnj_decision nnj_build_decision_none();
struct nnj_decision nnj_build_decision_movement(float rotation, float forward_diff, float lateral_diff);
struct nnj_decision nnj_build_decision_attack(size_t target_handle);
struct nnj_decision nnj_build_decision_activate(size_t column, size_t line);
struct nnj_decision nnj_build_decision_throw();

struct nnj_decision_commit {
	size_t target_handle;
	struct nnj_decision decision;
};

struct bot_api {
	void *ninja_descriptor;

	void(NINJACLOWN_CALLCONV *log)(enum nnj_log_level level, const char *text);

	size_t(NINJACLOWN_CALLCONV *map_width)(void *ninja_data);
	size_t(NINJACLOWN_CALLCONV *map_height)(void *ninja_data);
	void(NINJACLOWN_CALLCONV *map_scan)(void *ninja_data, struct nnj_cell *map_view);
	size_t(NINJACLOWN_CALLCONV *map_update)(void *ninja_data, struct nnj_cell *map_view, struct nnj_cell_pos *changed_cells,
	                                        size_t changed_size);

	size_t(NINJACLOWN_CALLCONV *max_entities)();
	void(NINJACLOWN_CALLCONV *entities_scan)(void *ninja_data, struct nnj_entity *entities);
	size_t(NINJACLOWN_CALLCONV *entities_update)(void *ninja_data, struct nnj_entity *entities);

	void(NINJACLOWN_CALLCONV *commit_decision)(void *ninja_data, struct nnj_decision_commit *commits, size_t num_commits);
};

#ifdef __cplusplus
} // extern "C"
} // namespace bot
#endif

#endif //NINJACLOWN_BOT_INTERFACE_BOT_H
