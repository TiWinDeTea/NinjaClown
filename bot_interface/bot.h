#ifndef NINJACLOWN_BOT_INTERFACE_BOT_H
#define NINJACLOWN_BOT_INTERFACE_BOT_H

#ifdef __cplusplus
namespace bot {
extern "C" {
#endif
struct bot_api {
	void (*log)(const char *);
	void (*go_right)();
};

#ifdef __cplusplus
} // extern "C"
} // namespace bot
#endif

#endif //NINJACLOWN_BOT_INTERFACE_BOT_H
