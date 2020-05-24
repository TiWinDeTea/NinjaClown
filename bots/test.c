#include <bot_interface/bot.h>
#include <bot_interface/helpers.h>

static _Bool finished;
static size_t ninja_clown_handle = 0;

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV on_start() {
	finished = 0;

	for (size_t i = 0; i < MAX_ENTITIES; ++i) {
		if (ENTITIES[i].kind == EK_DLL) {
			ninja_clown_handle = i;
		}
	}
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_init() {
	USER_START_LEVEL = on_start;
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_destroy() {
	nnj_log(LL_CRITICAL, "No... no... NO! DON'T DESTROY M");
}

void NINJACLOWN_DLLEXPORT NINJACLOWN_CALLCONV bot_think() {
	nnj_map_update();
	nnj_entities_update();

	struct nnj_decision_commit commit;
	commit.target_handle = ninja_clown_handle;
	commit.decision      = nnj_build_decision_none();

	if (finished) {
		commit.decision = nnj_build_decision_movement(1, 1, 0);
	}
	else {
		if (ENTITIES[ninja_clown_handle].angle < 2.8f) {
			commit.decision = nnj_build_decision_movement(-1, 0, 0);
		}
		else if (ENTITIES[ninja_clown_handle].x > 7.5) {
			commit.decision = nnj_build_decision_movement(0, 1, 0);
		}
		else {
			commit.decision = nnj_build_decision_activate(6, 1);
			finished        = 1;
			nnj_log(LL_INFO, "Me pushed button bip bop");
		}
	}

	nnj_commit_decision(&commit, 1);
}
