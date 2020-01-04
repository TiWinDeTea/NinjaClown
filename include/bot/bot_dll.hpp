#ifndef NINJACLOWN_BOT_DLL_HPP
#define NINJACLOWN_BOT_DLL_HPP

#include <optional>

#include "dll.hpp"

namespace bot {
struct bot_api; // forward declaration

using init_fn_type  = void (*)(bot_api *);
using think_fn_type = void (*)();

struct bot_dll {
	bot_dll() noexcept = default;
	explicit bot_dll(std::string dll_path) noexcept;
	explicit bot_dll(std::string &&dll_path) noexcept;

	explicit operator bool() const;

	[[nodiscard]] std::string error() const;
	[[nodiscard]] bool load(const std::string &dll_path) noexcept;
	[[nodiscard]] bool load(std::string &&dll_path) noexcept;
	[[nodiscard]] bool reload() noexcept;

	void bot_init(bot_api *api);
	void bot_think();

private:
	[[nodiscard]] bool load_all_api_functions();

	template <typename FuncPtr>
	bool try_load_function(FuncPtr &ptr, const char *func_name);

	std::optional<std::string> m_dll_path{};
	dll m_dll{};
	bool m_good{false};

	init_fn_type m_init_fn{};
	think_fn_type m_think_fn{};
};
} // namespace bot

#endif //NINJACLOWN_BOT_DLL_HPP
