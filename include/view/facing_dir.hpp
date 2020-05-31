#ifndef NINJACLOWN_VIEW_FACING_DIR_HPP
#define NINJACLOWN_VIEW_FACING_DIR_HPP

#include <array>
#include <optional>
#include <string_view>

#include "utils/utils.hpp"

namespace view::facing_direction {

enum type : unsigned int { N, S, E, W, NE, NW, SE, SW, MAX_VAL };

constexpr std::array values = {N, S, E, W, NE, NW, SE, SW, MAX_VAL};
static_assert(utils::has_all_sorted(values, MAX_VAL), "values array might not contain every enum value");

constexpr std::string_view to_string(facing_direction::type val) noexcept {
	constexpr std::array<std::string_view, MAX_VAL + 1> direction_map = {
	  "N", "S", "E", "W", "NE", "NW", "SE", "SW", "MAX_VAL",
	};
	return direction_map[val];
}

// TODO: add NE/NW/SE/SW
type from_angle(float rad);

constexpr std::optional<facing_direction::type> from_string(std::string_view str) noexcept {
	for (auto val : values) {
		if (to_string(val) == str) {
			return val;
		}
	}
	return {};
}
} // namespace view::facing_direction

#endif //NINJACLOWN_VIEW_FACING_DIR_HPP
