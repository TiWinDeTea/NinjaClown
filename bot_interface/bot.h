#ifndef NINJACLOWN_BOT_INTERFACE_BOT_H
#define NINJACLOWN_BOT_INTERFACE_BOT_H

#ifdef OS_WINDOWS
#	define NINJACLOWN_DLLEXPORT __declspec(dllexport)
#	define NINJACLOWN_CALLCONV  __cdecl
#else
#	define NINJACLOWN_DLLEXPORT
#	define NINJACLOWN_CALLCONV  __attribute((sysv_abi))
#endif

#ifdef __cplusplus
namespace bot {
extern "C" {
#endif

enum cell_type {
	CHASM  = 0,
	GROUND = 1,
	WALL   = 2,
};

enum interaction_kind {
	LIGHT_MANUAL   = 0, // character or thrown item can interact
	HEAVY_MANUAL   = 1, // only a character can interact
	LIGHT_MIDAIR   = 2, // character or thrown item in the cell cause interaction
	HEAVY_MIDAIR   = 3, // only character in the cell cause interaction
	WALK_ON_GROUND = 4, // only non-floating character in the cell cause interaction
	NO_INTERACTION = 10, // not an interactive cell
};

struct cell {
	enum cell_type type;
	enum interaction_kind interaction;
};

struct bot_api {
	void(NINJACLOWN_CALLCONV *log)(const char *);

	struct cell **(NINJACLOWN_CALLCONV *vision)();

	float(NINJACLOWN_CALLCONV *get_angle)();
	float(NINJACLOWN_CALLCONV *get_x_position)();
	float(NINJACLOWN_CALLCONV *get_y_position)();

	void(NINJACLOWN_CALLCONV *turn_left)();
	void(NINJACLOWN_CALLCONV *turn_right)();
	void(NINJACLOWN_CALLCONV *move_forward)();
	void(NINJACLOWN_CALLCONV *move_backward)();

	void(NINJACLOWN_CALLCONV *activate_button)();
};

#ifdef __cplusplus
} // extern "C"
} // namespace bot
#endif

#endif //NINJACLOWN_BOT_INTERFACE_BOT_H
