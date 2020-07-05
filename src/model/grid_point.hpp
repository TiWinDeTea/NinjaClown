#ifndef NINJACLOWN_MODEL_GRID_POINT_HPP
#define NINJACLOWN_MODEL_GRID_POINT_HPP

#include "utils/utils.hpp"

namespace model {
struct grid_point {
	utils::ssize_t x;
	utils::ssize_t y;

	[[nodiscard]] constexpr friend bool operator==(const grid_point &lhs, const grid_point &rhs) noexcept {
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	[[nodiscard]] constexpr static grid_point max() noexcept {
		constexpr auto max = std::numeric_limits<utils::ssize_t>::max();
		return {max, max};
	}

	[[nodiscard]] constexpr static grid_point min() noexcept {
		constexpr auto min = std::numeric_limits<utils::ssize_t>::min();
		return {min, min};
	}

	[[nodiscard]] constexpr static grid_point zero() noexcept {
		return {0, 0};
	}
};
} // namespace model

#endif //NINJACLOWN_MODEL_GRID_POINT_HPP
