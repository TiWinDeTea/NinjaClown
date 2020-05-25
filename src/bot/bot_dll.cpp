#include <spdlog/spdlog.h>

#include "bot/bot_dll.hpp"

bot::bot_dll::~bot_dll() {
	if (m_destroy_fn) {
		m_destroy_fn();
	}
}

bot::bot_dll::bot_dll(std::string dll_path) noexcept
    : m_dll_path{std::move(dll_path)} {
	static_cast<void>(reload());
}

bot::bot_dll::bot_dll(std::string &&dll_path) noexcept
    : m_dll_path{std::move(dll_path)} {
	static_cast<void>(reload());
}

bot::bot_dll::operator bool() const {
	return m_good;
}

std::string bot::bot_dll::error() const {
	return m_dll.error();
}

bool bot::bot_dll::load(const std::string &dll_path) noexcept {
	m_dll_path = {dll_path};
	return reload();
}

bool bot::bot_dll::load(std::string &&dll_path) noexcept {
	m_dll_path = {std::move(dll_path)};
	return reload();
}

bool bot::bot_dll::reload() noexcept {
	m_good = false;

	if (!m_dll_path) {
		spdlog::error("Attempted to reload bot dll without any path");
		return false;
	}

	if (!m_dll.load(*m_dll_path)) {
		spdlog::error("Failed to load: {}", m_dll.error());
		return false;
	}

	if (!load_all_api_functions()) {
		spdlog::error("Failed to load {}", *m_dll_path);
		return false;
	}

	m_good = true;
	return true;
}

void bot::bot_dll::bot_init() noexcept {
	if (m_init_fn) {
		m_init_fn();
	}

	if (m_cached_bot_api) {
		bot_start_level(*m_cached_bot_api);
	}
}

void bot::bot_dll::bot_start_level(ninja_api::nnj_api api) noexcept {
	if (m_start_level_fn) {
		m_start_level_fn(api);
	}
	else {
		m_cached_bot_api = {api};
	}
}

void bot::bot_dll::bot_think() noexcept {
	m_think_fn();
}

void bot::bot_dll::bot_end_level() noexcept {
	if (m_end_level_fn) {
		m_end_level_fn();
	}
}

bool bot::bot_dll::load_all_api_functions() {
	bool good;

	good = try_load_function(m_start_level_fn, "bot_start_level", true);
	try_load_function(m_think_fn, "bot_think", true) && good;

	try_load_function(m_init_fn, "bot_init", false);
	try_load_function(m_end_level_fn, "bot_end_level", false);
	try_load_function(m_destroy_fn, "bot_destroy", false);

	return good;
}

template <typename FuncPtr>
bool bot::bot_dll::try_load_function(FuncPtr &ptr, const char *func_name, bool required) {
	ptr = m_dll.get_address<FuncPtr>(func_name);
	if (ptr == nullptr) {
		if (required) {
			spdlog::error("Failed to load {} from dll {}", func_name, *m_dll_path);
		}
		else {
			spdlog::info("Optional {} function not found in {}", func_name, *m_dll_path);
		}
		return false;
	}
	return true;
}
