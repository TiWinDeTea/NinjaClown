#include "model/interactables/button.hpp"

#include "program_state.hpp"
#include "view/map.hpp"
#include "view/viewer.hpp"

void model::button_system(button &button, std::vector<std::vector<cell>> &grid) {
	cell_type &current = grid[button.target.column][button.target.row].type;
	cell_type new_value;
	if (current == cell_type::GROUND) {
		new_value = cell_type::WALL;
	}
	else {
		new_value = cell_type::GROUND;
	}
	current = new_value;
	program_state::global->adapter.update_map(button.target.column, button.target.row, new_value);
}
