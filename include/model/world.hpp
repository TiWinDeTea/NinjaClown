#ifndef NINJACLOWN_WORLD_HPP
#define NINJACLOWN_WORLD_HPP

#include <algorithm> // max, min
#include <optional>
#include <string>
#include <vector>

#include "cell.hpp"
#include "components.hpp"
#include "grid.hpp"
#include "interactables/button.hpp"
#include "interaction.hpp"

namespace model {

struct world {
	world() noexcept = default;

	void update(adapter::adapter &);

	grid_t grid{};

	size_t ninja_clown_handle{};

	model::components components{};

	std::vector<interaction> interactions{};
	std::vector<button> buttons{};
};

} // namespace model

#endif //NINJACLOWN_WORLD_HPP
