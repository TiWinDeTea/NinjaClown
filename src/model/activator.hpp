#ifndef NINJACLOWN_MODEL_ACTIVATORS_HPP
#define NINJACLOWN_MODEL_ACTIVATORS_HPP

#include <cassert>
#include <vector>

#include "utils/utils.hpp"
#include "model/grid.hpp"

namespace model {

struct activator {
    std::vector<size_t> targets; // corresponds to world::actionables
    std::optional<unsigned int> refire_after; // TODO unused
	unsigned int activation_delay; // TODO unused
};
}; // namespace model


#endif //NINJACLOWN_MODEL_ACTIVATORS_HPP
