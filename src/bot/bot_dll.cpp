#include <spdlog/spdlog.h>

#include "bot/bot_dll.hpp"

template <typename FuncPtr>
bool bot::bot_dll::h_try_load(FuncPtr &ptr, const char *func_name)
{
	ptr = m_dll.get_address<FuncPtr>(func_name);
	if (ptr == nullptr) {
		spdlog::error("Failed to load {} from dll {}", func_name, m_dll_path);
		return false;
	}
	return true;
}

bot::bot_dll::bot_dll(const std::string &dll_path) noexcept
    : m_dll{dll_path}
    , m_dll_path{dll_path}
{
	m_good = m_dll && h_load_api();
}

bot::bot_dll::operator bool() const
{
	return m_good;
}

std::string bot::bot_dll::error() const
{
	return m_dll.error();
}

bool bot::bot_dll::reload() noexcept
{
	m_dll.reload(m_dll_path);
	m_good = m_dll && h_load_api();
	return m_good;
}

void bot::bot_dll::bot_init(bot::bot_api *api)
{
	m_init_fn(api);
}

void bot::bot_dll::bot_think()
{
	m_think_fn();
}

bool bot::bot_dll::h_load_api()
{
	if (m_dll) {
		m_good = h_try_load(m_init_fn, "bot_init");
		m_good = h_try_load(m_think_fn, "bot_think") && m_good;

		if (!m_good) {
			spdlog::error("Failed to load dll");
		}
		return m_good;
	}

	spdlog::error("Tried to load methods from non-loaded dll \"{}\".", m_dll_path);
	return false;
}
