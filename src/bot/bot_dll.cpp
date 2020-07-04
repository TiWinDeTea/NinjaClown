#include <spdlog/spdlog.h>

#include "bot/bot_dll.hpp"
#include "utils/logging.hpp"

using fmt::literals::operator""_a;

bot::bot_dll::~bot_dll() {
	if (m_destroy_fn) {
		m_destroy_fn();
	}
}

bot::bot_dll::operator bool() const {
	return m_good;
}

std::string bot::bot_dll::error() const {
	return m_dll.error();
}

bool bot::bot_dll::load(const utils::resource_manager& res, const std::string &dll_path) noexcept {
	m_dll_path = {dll_path};
	return reload(res);
}

bool bot::bot_dll::load(const utils::resource_manager& res, std::string &&dll_path) noexcept {
	m_dll_path = {std::move(dll_path)};
	return reload(res);
}

bool bot::bot_dll::reload(const utils::resource_manager& res) noexcept {
	m_good = false;

	if (!m_dll_path) {
		spdlog::error("Attempted to reload bot dll without any path");
		return false;
	}

	if (!m_dll.load(*m_dll_path)) {
		spdlog::error("Failed to load: {}", m_dll.error());
		return false;
	}

	if (!load_all_api_functions(res)) {
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
	m_cached_bot_api = {api};
	if (m_start_level_fn) {
		m_start_level_fn(api);
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

bool bot::bot_dll::load_all_api_functions(const utils::resource_manager& res) {
	bool good;

	good = try_load_function(res, m_start_level_fn, "bot_start_level", true);
	good = try_load_function(res, m_think_fn, "bot_think", true) && good;

	try_load_function(res, m_init_fn, "bot_init", false);
	try_load_function(res, m_end_level_fn, "bot_end_level", false);
	try_load_function(res, m_destroy_fn, "bot_destroy", false);

	return good;
}

template <typename FuncPtr>
bool bot::bot_dll::try_load_function(const utils::resource_manager& res, FuncPtr &ptr, const char *func_name, bool required) {
	ptr = m_dll.get_address<FuncPtr>(func_name);
	if (ptr == nullptr) {
		if (required) {
			utils::log::error(res, "bot_dll.required_load_failed", "func_name"_a = func_name, "file"_a = *m_dll_path);
		}
		else {
            utils::log::info(res, "bot_dll.optional_load_failed", "func_name"_a = func_name, "file"_a = *m_dll_path);
		}
		return false;
	} else {
		if (!required) {
            utils::log::info(res, "bot_dll.optional_load_success", "func_name"_a = func_name, "file"_a = *m_dll_path);
		}
	}
	return true;
}
