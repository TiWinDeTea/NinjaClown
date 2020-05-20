#ifndef NINJACLOWN_BUTTON_HPP
#define NINJACLOWN_BUTTON_HPP

#include <cstddef>
#include <vector>
#include <view/map.hpp>

#include "model/cell.hpp"
#include "model/components.hpp"
#include "model/grid.hpp"

namespace model {

struct button {
	struct target_t {
		ssize_t column;
		ssize_t row;
	};

	std::vector<target_t> targets;
};

void toggle_button(adapter::adapter &adapter, const button &button, grid_t &grid);

} // namespace model

#endif //NINJACLOWN_BUTTON_HPP
