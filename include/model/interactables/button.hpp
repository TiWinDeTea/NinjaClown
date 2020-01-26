#ifndef NINJACLOWN_BUTTON_HPP
#define NINJACLOWN_BUTTON_HPP

#include <cstddef>
#include <vector>
#include <view/map.hpp>

#include "model/cell.hpp"
#include "model/components.hpp"

namespace model {

struct button {
	struct {
		size_t column;
		size_t row;
	} target;
};

void toggle_button(button &button, std::vector<std::vector<cell>> &grid);

} // namespace model

#endif //NINJACLOWN_BUTTON_HPP
