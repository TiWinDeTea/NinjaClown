#ifndef NINJACLOWN_WORLD_HPP
#define NINJACLOWN_WORLD_HPP

#include <algorithm> // max, min
#include <optional>
#include <string>
#include <vector>

#include "cell.hpp"
#include "components.hpp"
#include "grid_iterator.hpp"
#include "interactables/button.hpp"
#include "interaction.hpp"

namespace model {

struct world {
	world() noexcept = default;

	void update(adapter::adapter &);

	std::vector<std::vector<cell>> grid{};

	size_t ninja_clown_handle{};

	model::components components{};

	std::vector<interaction> interactions{};
	std::vector<button> buttons{};

	grid_iterator iterate_grid(float center_x, float center_y, float radius) {
		auto start_x  = std::max<size_t>(0, static_cast<size_t>(center_x - radius));
		auto start_y  = std::max<size_t>(0, static_cast<size_t>(center_y - radius));
		auto target_x = std::min<size_t>(grid.size() - 1, static_cast<size_t>(center_x + radius));
		auto target_y = std::min<size_t>(grid[0].size() - 1, static_cast<size_t>(center_y + radius));

		return grid_iterator{grid, start_x, start_y, target_x, target_y};
	}
};

} // namespace model

#endif //NINJACLOWN_WORLD_HPP
