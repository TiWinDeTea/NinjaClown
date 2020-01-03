#include <iostream> // FIXME

#include "bot/bot_api.hpp"

void bot::ffi::log(const char* text) {
	std::cout << "[bot] " << text << "\n";
}

void bot::ffi::go_right() {
	std::cout << "go right\n";
}

void bot::ffi::go_right_dummy() {
	std::cout << "go right dummy\n";
}
