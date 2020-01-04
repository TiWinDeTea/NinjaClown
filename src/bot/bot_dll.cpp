#include <spdlog/spdlog.h>

#include "bot/bot_dll.hpp"

bot::bot_dll::bot_dll(std::string dll_path) noexcept
    : m_dll_path{std::move(dll_path)}
{
	static_cast<void>(reload());
}

bot::bot_dll::bot_dll(std::string &&dll_path) noexcept
    : m_dll_path{std::move(dll_path)}
{
	static_cast<void>(reload());
}

bot::bot_dll::operator bool() const
{
	return m_good;
}

std::string bot::bot_dll::error() const
{
	return m_dll.error();
}

bool bot::bot_dll::load(const std::string &dll_path) noexcept
{
	m_dll_path = {dll_path};
	return reload();
}

bool bot::bot_dll::load(std::string &&dll_path) noexcept
{
	m_dll_path = {std::move(dll_path)};
	return reload();
}

bool bot::bot_dll::reload() noexcept
{
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

void bot::bot_dll::bot_init(bot::bot_api *api)
{
	m_init_fn(api);
}

void bot::bot_dll::bot_think()
{
	m_think_fn();
}

bool bot::bot_dll::load_all_api_functions()
{
	bool good;

	good = try_load_function(m_init_fn, "bot_init");
	good = try_load_function(m_think_fn, "bot_think") && good;

	return good;
}

template <typename FuncPtr>
bool bot::bot_dll::try_load_function(FuncPtr &ptr, const char *func_name)
{
	ptr = m_dll.get_address<FuncPtr>(func_name);
	if (ptr == nullptr) {
		spdlog::error("Failed to load {} from dll {}", func_name, *m_dll_path);
		return false;
	}
	return true;
}
