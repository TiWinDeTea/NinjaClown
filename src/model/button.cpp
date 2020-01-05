#include "model/interactables/button.hpp"

#include "program_state.hpp"
#include "view/map.hpp"
#include "view/viewer.hpp"

void model::button_system(button &button, std::vector<std::vector<cell>> &grid)
{
	if (grid[button.target.column][button.target.row].type == cell_type::GROUND) {
		grid[button.target.column][button.target.row].type = cell_type::WALL;
		program_state::global->viewer.acquire_map()->set_cell(button.target.row, button.target.column, view::map::cell::abyss);
	}
	else {
		grid[button.target.column][button.target.row].type = cell_type::GROUND;
		program_state::global->viewer.acquire_map()->set_cell(button.target.row, button.target.column, view::map::cell::concrete_tile);
	}
}
