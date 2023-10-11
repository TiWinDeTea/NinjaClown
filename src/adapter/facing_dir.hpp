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

type from_angle(float rad);

} // namespace view::facing_direction

#endif //NINJACLOWN_VIEW_FACING_DIR_HPP
