#include <spdlog/spdlog.h>

#include "bot/bot_dll.hpp"


template <typename FuncPtr>
bool bot::bot_dll::try_load(FuncPtr& ptr, const char* func_name) {
    ptr = m_dll.get_address<FuncPtr>("bot_init");
    if (ptr == nullptr) {
		spdlog::error("Failed to load {} from dll {}", func_name, m_dll_path);
		return false;
	}
    return true;
}

bot::bot_dll::bot_dll(const std::string& dll_path) noexcept: m_dll{dll_path}, m_dll_path{dll_path} {
    good = m_dll && h_load_api();
}

bot::bot_dll::operator bool() const {
	return good;
}

std::string bot::bot_dll::error() const{
    return m_dll.error();
}

bool bot::bot_dll::reload() noexcept {
    m_dll.reload(m_dll_path);
    good = m_dll && h_load_api();
	return good;
}

void bot::bot_dll::bot_init(bot::bot_api *api){
	m_init_fn(api);
}

void bot::bot_dll::bot_think(){
	m_think_fn();
}

bool bot::bot_dll::h_load_api(){
	if (*this) {
	    bool success = try_load(m_init_fn, "bot_init");
		success = try_load(m_think_fn, "bot_think") && success;

		if (!success) {
		    spdlog::error("Failed to load dll");
		}
		return success;
	}

	spdlog::error("Tried to load methods from non-loaded dll \"{}\".", m_dll_path);
	return false;
}

