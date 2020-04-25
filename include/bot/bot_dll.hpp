#ifndef NINJACLOWN_BOT_DLL_HPP
#define NINJACLOWN_BOT_DLL_HPP

#include <bot_interface/bot.h>
#include <optional>

#include "utils/dll.hpp"

namespace bot {
struct bot_api; // forward declaration

using init_fn_type        = void(NINJACLOWN_CALLCONV *)();
using start_level_fn_type = void(NINJACLOWN_CALLCONV *)(bot_api);
using think_fn_type       = void(NINJACLOWN_CALLCONV *)();
using destroy_fn_type     = void(NINJACLOWN_CALLCONV *)();

struct bot_dll {
	bot_dll() noexcept = default;
	explicit bot_dll(std::string dll_path) noexcept;
	explicit bot_dll(std::string &&dll_path) noexcept;

	explicit operator bool() const;

	[[nodiscard]] std::string error() const;
	[[nodiscard]] bool load(const std::string &dll_path) noexcept;
	[[nodiscard]] bool load(std::string &&dll_path) noexcept;
	[[nodiscard]] bool reload() noexcept;

	void bot_init() noexcept;
	void bot_start_level(bot_api api) noexcept;
	void bot_think() noexcept;
	void bot_destroy() noexcept;

private:
	[[nodiscard]] bool load_all_api_functions();

	template <typename FuncPtr>
	bool try_load_function(FuncPtr &ptr, const char *func_name, bool required);

	std::optional<std::string> m_dll_path{};
	utils::dll m_dll{};
	bool m_good{false};

	init_fn_type m_init_fn{};
	start_level_fn_type m_start_level_fn{};
	think_fn_type m_think_fn{};
	destroy_fn_type m_destroy_fn{};

	std::optional<bot_api> m_cached_bot_api;
};
} // namespace bot

#endif //NINJACLOWN_BOT_DLL_HPP
