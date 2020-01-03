#include <spdlog/spdlog.h>

#include "bot/bot_api.hpp"

void bot::ffi::log(const char* text) {
    spdlog::info("[bot]: {}", text);
}

void bot::ffi::go_right() {
    spdlog::info("[bot is going right]");
}

void bot::ffi::go_right_dummy() {
    spdlog::error("[called go right, but the bot does not know how to do that!]");
}
