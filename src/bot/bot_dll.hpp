#ifndef NINJACLOWN_BOT_DLL_HPP
#define NINJACLOWN_BOT_DLL_HPP

#include <optional>

#include <ninja_clown/api.h>

#include "utils/dll.hpp"

namespace utils {
class resource_manager;
}

namespace bot {

using init_fn_type        = void(NINJACLOWN_CALLCONV *)();
using start_level_fn_type = void(NINJACLOWN_CALLCONV *)(ninja_api::nnj_api);
using think_fn_type       = void(NINJACLOWN_CALLCONV *)();
using end_level_fn_type   = void(NINJACLOWN_CALLCONV *)();
using destroy_fn_type     = void(NINJACLOWN_CALLCONV *)();

struct bot_dll {
	bot_dll() noexcept = default;
	~bot_dll();

	explicit operator bool() const;

	[[nodiscard]] std::string error() const;
	[[nodiscard]] bool load(const utils::resource_manager&, const std::string &dll_path) noexcept;
	[[nodiscard]] bool load(const utils::resource_manager&, std::string &&dll_path) noexcept;
	[[nodiscard]] bool reload(const utils::resource_manager&) noexcept;

	void bot_init() noexcept;
	void bot_start_level(ninja_api::nnj_api api) noexcept;
	void bot_think() noexcept;
	void bot_end_level() noexcept;

private:
	[[nodiscard]] bool load_all_api_functions(const utils::resource_manager&);

	template <typename FuncPtr>
	bool try_load_function(const utils::resource_manager&, FuncPtr &ptr, const char *func_name, bool required);

	void reset();

	std::optional<std::string> m_dll_path{};
	utils::dll m_dll{};
	bool m_good{false};

	init_fn_type m_init_fn{};
	start_level_fn_type m_start_level_fn{};
	think_fn_type m_think_fn{};
	end_level_fn_type m_end_level_fn{};
	destroy_fn_type m_destroy_fn{};

	std::optional<ninja_api::nnj_api> m_cached_bot_api;
};
} // namespace bot

#endif //NINJACLOWN_BOT_DLL_HPP
