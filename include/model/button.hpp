#ifndef NINJACLOWN_BUTTON_HPP
#define NINJACLOWN_BUTTON_HPP

#include <cstddef>
#include <vector>

#include "cell.hpp"

namespace model {
struct button {
	struct {
		size_t column;
		size_t row;
	} target;
};

inline void button_system(button &button, std::vector<std::vector<cell>> &grid)
{
	if (grid[button.target.column][button.target.row].type == cell_type::GROUND) {
		grid[button.target.column][button.target.row].type = cell_type::WALL;
	}
	else {
		grid[button.target.column][button.target.row].type = cell_type::GROUND;
	}
}
} // namespace model

#endif //NINJACLOWN_BUTTON_HPP
