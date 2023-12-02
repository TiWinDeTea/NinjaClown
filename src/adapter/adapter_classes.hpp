#ifndef NINJACLOWN_ADAPTER_ADAPTER_CLASSES_HPP
#define NINJACLOWN_ADAPTER_ADAPTER_CLASSES_HPP

#include <vector>

#include "model/types.hpp"
#include "ninja_clown/api.h"
#include "utils/utils.hpp"

namespace adapter {
enum class bot_log_level {
	BTRACE    = 0,
	BDEBUG    = 1,
	BINFO     = 2,
	BWARN     = 3,
	BERROR    = 4,
	BCRITICAL = 5,
};

struct model_handle {
	enum type_t {
		ACTIVATOR  = 1,
		ACTIONABLE = 2,
		ENTITY     = 4, // TODO : should it be 1 << 2 or 0b00100 ?
	};

	explicit model_handle() noexcept = default;
	explicit constexpr model_handle(model::handle_t handle_, type_t type_) noexcept
	    : handle{handle_}
	    , type{type_} { }
	constexpr bool operator==(const model_handle &other) const noexcept {
		return other.handle == handle && type == other.type;
	}

	model::handle_t handle{};
	type_t type{};
};

struct view_handle {
	explicit view_handle() noexcept = default;
	explicit constexpr view_handle(bool is_mob_, size_t handle_) noexcept
	    : is_mob{is_mob_}
	    , handle{handle_} { }
	constexpr bool operator==(const view_handle &other) const noexcept {
		return other.is_mob == is_mob && other.handle == handle;
	}
	bool is_mob{};
	size_t handle{};
};

struct model_hhash: private std::hash<utils::ssize_t> {
	utils::ssize_t operator()(const model_handle &h) const noexcept {
		return std::hash<utils::ssize_t>::operator()(h.handle | h.type << 16u);
	}
};
struct view_hhash {
	std::size_t operator()(const view_handle &h) const noexcept;
};

namespace request {
	struct coords {
		std::size_t x, y;
	};

	struct hitbox {
		float x, y;
		float width, height;
	};

	struct info {
		std::vector<std::string> lines;
	};
} // namespace request

struct angle {
	float val;
};

struct behaviour {
	enum class bhvr {
		harmless,
		patrol,
		aggressive,
		dll,
		unsupported
	} val;
};

struct toggler_targets {
	std::vector<std::string> names;
	std::basic_string<bool> values;
};

using entity_edit_v = std::variant<std::string, float, std::uint8_t, angle, behaviour, toggler_targets>;
using entity_edit   = std::vector<std::pair<std::string, entity_edit_v>>;
} // namespace adapter

#endif //NINJACLOWN_ADAPTER_ADAPTER_CLASSES_HPP
