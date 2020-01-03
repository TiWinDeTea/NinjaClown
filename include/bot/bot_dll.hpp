#ifndef NINJACLOWN_BOT_DLL_HPP
#define NINJACLOWN_BOT_DLL_HPP

#include <optional>

#include "dll.hpp"

namespace bot {
struct bot_api; // forward declaration

using init_fn_type = void (*)(bot_api*);
using think_fn_type = void (*)();

struct bot_dll {
	explicit bot_dll(const std::string &dll_path) noexcept;

	explicit operator bool() const;

	[[nodiscard]] std::string error() const;
	[[nodiscard]] bool reload() noexcept;

	void bot_init(bot_api* api);
	void bot_think();

private:
	[[nodiscard]] bool h_load_api();

    template <typename FuncPtr>
    bool try_load(FuncPtr& ptr, const char* func_name);

	std::string m_dll_path;
	dll m_dll;
	bool good{false};

	init_fn_type m_init_fn{};
	think_fn_type m_think_fn{};
};
}

#endif //NINJACLOWN_BOT_DLL_HPP
