#include "bot/bot_dll.hpp"

bot::bot_dll::bot_dll(const std::string& dll_path) noexcept: m_dll{dll_path}, m_dll_path{dll_path} {
	h_load_api();
}

bot::bot_dll::operator bool() const {
	return m_dll && m_dll.value();
}

std::string bot::bot_dll::error() const{
	if (m_dll) {
		return m_dll->error();
	} else {
		return "";
	}
}

void bot::bot_dll::reload() noexcept {
	if (m_dll) {
		m_dll->reload(m_dll_path);
	} else {
		m_dll = { dll(m_dll_path) };
	}

	h_load_api();
}

void bot::bot_dll::bot_init(bot::bot_api *api){
	m_init_fn(api);
}

void bot::bot_dll::bot_think(){
	m_think_fn();
}

void bot::bot_dll::h_load_api(){
	if (*this) {
		m_init_fn = m_dll->get_address<init_fn_type>("bot_init");
		m_think_fn = m_dll->get_address<think_fn_type>("bot_think");
	}
}

