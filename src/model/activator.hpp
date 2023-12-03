#ifndef NINJACLOWN_MODEL_ACTIVATORS_HPP
#define NINJACLOWN_MODEL_ACTIVATORS_HPP

#include <cassert>
#include <vector>

#include "model/grid.hpp"
#include "model/types.hpp"
#include "utils/utils.hpp"

namespace model {

constexpr tick_t default_activation_delay      = 0;
constexpr tick_t default_activation_difficulty = 10;
constexpr unsigned int default_firing_rate     = 1u;

struct activator {
	std::vector<size_t> targets; // corresponds to world::actionables
	std::optional<tick_t> refire_after;
	tick_t activation_delay      = default_activation_delay;
	tick_t activation_difficulty = default_activation_difficulty;
	bool refire_repeat           = false;
	bool enabled                 = false; // used to determine if refire or delay should be cancelled
};
}; // namespace model

#endif //NINJACLOWN_MODEL_ACTIVATORS_HPP
