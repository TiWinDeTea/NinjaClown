#ifndef NINJACLOWN_UTILS_LOGGING_HPP
#define NINJACLOWN_UTILS_LOGGING_HPP

#include <spdlog/spdlog.h>
#include <string_view>
#include <utility>

#include "utils/optional.hpp"

namespace utils::log {
namespace details {
	[[nodiscard]] std::string_view log_for(std::string_view key) noexcept;
}

template <typename... Args>
void trace(std::string_view fmt_key, Args &&... args) {
    spdlog::trace(details::log_for(fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void debug(std::string_view fmt_key, Args &&... args) {
    spdlog::debug(details::log_for(fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void info(std::string_view fmt_key, Args &&... args) {
    spdlog::info(details::log_for(fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void warn(std::string_view fmt_key, Args &&... args) {
    spdlog::warn(details::log_for(fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void error(std::string_view fmt_key, Args &&... args) {
    spdlog::error(details::log_for(fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void critical(std::string_view fmt_key, Args &&... args) {
    spdlog::critical(details::log_for(fmt_key), std::forward<Args>(args)...);
}
} // namespace utils::log

#endif //NINJACLOWN_UTILS_LOGGING_HPP
