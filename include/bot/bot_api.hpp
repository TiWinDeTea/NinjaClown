#ifndef NINJACLOWN_BOT_API_HPP
#define NINJACLOWN_BOT_API_HPP

namespace bot {
extern "C" {
struct bot_api {
	void (*log)(const char*);
	void (*go_right)();
};
}

namespace ffi {
	void log(const char* text);

	void go_right();
	void go_right_dummy();
}
}

#endif //NINJACLOWN_BOT_API_HPP
