#include "model/interactables/button.hpp"

#include "adapter/adapter.hpp"
#include "view/map.hpp"
#include "view/viewer.hpp"

void model::toggle_button(adapter::adapter &adapter, const button &button, grid_t &grid) {
	for (const button::target_t& target : button.targets) {
		cell_type &current = grid[target.column][target.row].type;
		cell_type new_value;
		if (current == cell_type::GROUND) {
			new_value = cell_type::WALL;
		}
		else {
			new_value = cell_type::GROUND;
		}
		current = new_value;
		adapter.update_map(target.column, target.row, new_value);
	}
}
