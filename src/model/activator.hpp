#ifndef NINJACLOWN_MODEL_ACTIVATORS_HPP
#define NINJACLOWN_MODEL_ACTIVATORS_HPP

#include <cassert>
#include <vector>

#include "model/grid.hpp"
#include "model/types.hpp"
#include "utils/utils.hpp"

namespace model {

struct activator {
	std::vector<size_t> targets; // corresponds to world::actionables
	std::optional<tick_t> refire_after; // TODO unused
	tick_t activation_delay; // TODO unused
	tick_t activation_difficulty;
};
}; // namespace model

#endif //NINJACLOWN_MODEL_ACTIVATORS_HPP
