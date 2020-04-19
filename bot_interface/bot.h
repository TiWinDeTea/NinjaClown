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
#include <cstddef>
namespace bot {
extern "C" {
#else
#include <stddef.h>
#endif

enum cell_type {
    UNKNOWN = 0,
	CHASM   = 1,
	GROUND  = 2,
	WALL    = 3,
};

enum interaction_kind {
    NO_INTERACTION = 0, // not an interactive cell
	LIGHT_MANUAL   = 1, // character or thrown item can interact
	HEAVY_MANUAL   = 2, // only a character can interact
	LIGHT_MIDAIR   = 3, // character or thrown item in the cell cause interaction
	HEAVY_MIDAIR   = 4, // only character in the cell cause interaction
	WALK_ON_GROUND = 5, // only non-floating character in the cell cause interaction
};

struct cell {
	enum cell_type type;
	enum interaction_kind interaction;
};

struct bot_api {
	void *ninja_descriptor;

	void(NINJACLOWN_CALLCONV *log)(const char *);

    size_t(NINJACLOWN_CALLCONV *map_width)(void* ninja_data);
    size_t(NINJACLOWN_CALLCONV *map_height)(void* ninja_data);
	void(NINJACLOWN_CALLCONV *map_scan)(void *ninja_data, struct cell *map_view);
    void(NINJACLOWN_CALLCONV *map_update)(void *ninja_data, struct cell *map_view);

	float(NINJACLOWN_CALLCONV *get_angle)(void *ninja_data);
	float(NINJACLOWN_CALLCONV *get_x_position)(void *ninja_data);
	float(NINJACLOWN_CALLCONV *get_y_position)(void *ninja_data);

	void(NINJACLOWN_CALLCONV *turn_left)(void *ninja_data);
	void(NINJACLOWN_CALLCONV *turn_right)(void *ninja_data);
	void(NINJACLOWN_CALLCONV *move_forward)(void *ninja_data);
	void(NINJACLOWN_CALLCONV *move_backward)(void *ninja_data);

	void(NINJACLOWN_CALLCONV *activate_button)(void *ninja_data);
};

#ifdef __cplusplus
} // extern "C"
} // namespace bot
#endif

#endif //NINJACLOWN_BOT_INTERFACE_BOT_H
