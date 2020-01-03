#include "bot_interface/bot.h"

static struct bot_api* BOT;

#ifdef OS_WINDOWS
#define NINJACLOWN_DLLEXPORT __declspec(dllexport)
#else
#define NINJACLOWN_DLLEXPORT
#endif

void NINJACLOWN_DLLEXPORT bot_init(struct bot_api* api) {
	BOT = api;
}

void NINJACLOWN_DLLEXPORT bot_think() {
	BOT->log("hello from test bot!");
	BOT->go_right();
}
