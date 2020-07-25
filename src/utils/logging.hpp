#ifndef NINJACLOWN_UTILS_LOGGING_HPP
#define NINJACLOWN_UTILS_LOGGING_HPP

#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>

#include "utils/optional.hpp"

namespace utils {
class resource_manager;
}

namespace utils::log {
namespace details {
	[[nodiscard]] std::string_view log_for(const resource_manager &, std::string_view key) noexcept;
}

template <typename... Args>
void trace(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
    spdlog::trace(details::log_for(rm, fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void debug(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
    spdlog::debug(details::log_for(rm, fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void info(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
    spdlog::info(details::log_for(rm, fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void warn(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
    spdlog::warn(details::log_for(rm, fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void error(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
    spdlog::error(details::log_for(rm, fmt_key), std::forward<Args>(args)...);
}

template <typename... Args>
void critical(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
    spdlog::critical(details::log_for(rm, fmt_key), std::forward<Args>(args)...);
}
} // namespace utils::log

#endif //NINJACLOWN_UTILS_LOGGING_HPP
