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
	[[nodiscard]] utils::optional<std::string_view> log_for(const resource_manager &, std::string_view key) noexcept;
}

template <typename... Args>
bool trace(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
	utils::optional<std::string_view> fmt = details::log_for(rm, fmt_key);
	if (fmt) {
		spdlog::trace(*fmt, std::forward<Args>(args)...);
		return true;
	}
	else {
		spdlog::trace("Unknown log key: {}", fmt_key);
		return false;
	}
}

template <typename... Args>
bool debug(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
	utils::optional<std::string_view> fmt = details::log_for(rm, fmt_key);
	if (fmt) {
		spdlog::debug(*fmt, std::forward<Args>(args)...);
		return true;
	}
	else {
		spdlog::debug("Unknown log key: {}", fmt_key);
		return false;
	}
}

template <typename... Args>
bool info(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
	utils::optional<std::string_view> fmt = details::log_for(rm, fmt_key);
	if (fmt) {
		spdlog::info(*fmt, std::forward<Args>(args)...);
		return true;
	}
	else {
		spdlog::info("Unknown log key: {}", fmt_key);
		return false;
	}
}

template <typename... Args>
bool warn(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
	utils::optional<std::string_view> fmt = details::log_for(rm, fmt_key);
	if (fmt) {
		spdlog::warn(*fmt, std::forward<Args>(args)...);
		return true;
	}
	else {
		spdlog::warn("Unknown log key: {}", fmt_key);
		return false;
	}
}

template <typename... Args>
bool error(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
	utils::optional<std::string_view> fmt = details::log_for(rm, fmt_key);
	if (fmt) {
		spdlog::error(*fmt, std::forward<Args>(args)...);
		return true;
	}
	else {
		spdlog::error("Unknown log key: {}", fmt_key);
		return false;
	}
}

template <typename... Args>
bool critical(const resource_manager &rm, std::string_view fmt_key, Args &&... args) {
	utils::optional<std::string_view> fmt = details::log_for(rm, fmt_key);
	if (fmt) {
		spdlog::critical(*fmt, std::forward<Args>(args)...);
		return true;
	}
	else {
		spdlog::critical("Unknown log key: {}", fmt_key);
		return false;
	}
}

[[nodiscard]] std::string get_or_gen(const resource_manager &rm, std::string_view key);
} // namespace utils::log

#endif //NINJACLOWN_UTILS_LOGGING_HPP
