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
namespace ninja_api {
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

enum nnj_cell_kind {
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
	enum nnj_cell_kind kind;
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

struct nnj_properties {
	float move_speed;
	float rotation_speed;
	float attack_range;
	float activate_range;
	float attack_delay;
	float throw_delay;
};

enum nnj_entity_state {
	ES_READY = 0, // entity is ready for any action
	ES_BUSY  = 1, // entity is busy and cannot perform any other action
};

struct nnj_entity {
	enum nnj_entity_kind kind;
	enum nnj_entity_state state;
	struct nnj_properties properties;
	float x, y;
	float angle;
	size_t handle;
};

enum nnj_decision_kind {
	DK_NONE          = 0,
	DK_MOVEMENT      = 1,
	DK_ACTIVATE      = 2,
	DK_ATTACK        = 3,
	DK_THROW         = 4,
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

struct nnj_throw_request {
	void *unused;
};

struct nnj_decision {
	enum nnj_decision_kind kind;
	union {
		struct nnj_movement_request movement_req;
		struct nnj_activate_request activate_req;
		struct nnj_attack_request attack_req;
		struct nnj_throw_request throw_req;
	};
};

struct nnj_decision_commit {
	size_t target_handle;
	struct nnj_decision decision;
};

struct nnj_api {
	void *ninja_descriptor;

	void(NINJACLOWN_CALLCONV *log)(void *ninja_data, enum nnj_log_level level, const char *text);

	size_t(NINJACLOWN_CALLCONV *map_width)(void *ninja_data);
	size_t(NINJACLOWN_CALLCONV *map_height)(void *ninja_data);
	struct nnj_cell_pos(NINJACLOWN_CALLCONV *target_position)(void *ninja_data);
	void(NINJACLOWN_CALLCONV *map_scan)(void *ninja_data, struct nnj_cell *map_view);
	size_t(NINJACLOWN_CALLCONV *map_update)(void *ninja_data, struct nnj_cell *map_view, struct nnj_cell_pos *changed_cells,
	                                        size_t changed_size);

	size_t(NINJACLOWN_CALLCONV *max_entities)();
	void(NINJACLOWN_CALLCONV *entities_scan)(void *ninja_data, struct nnj_entity *entities);
	size_t(NINJACLOWN_CALLCONV *entities_update)(void *ninja_data, struct nnj_entity *entities);

	void(NINJACLOWN_CALLCONV *commit_decisions)(void *ninja_data, struct nnj_decision_commit const *commits, size_t num_commits);
};

#ifdef __cplusplus
} // extern "C"
} // namespace ninja_api
#endif

#endif //NINJACLOWN_BOT_INTERFACE_BOT_H
