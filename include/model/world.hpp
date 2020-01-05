#ifndef NINJACLOWN_WORLD_HPP
#define NINJACLOWN_WORLD_HPP

#include <optional>
#include <string>
#include <vector>

#include "cell.hpp"
#include "components.hpp"
#include "interactables/button.hpp"
#include "interaction.hpp"

namespace model {

struct world {
	world() noexcept = default;

	void load_map(const std::string &map_path);
	void update();

	std::vector<std::vector<cell>> grid{};

	size_t ninja_clown_handle{};

	model::components components{};

	std::vector<interaction> interactions{};
	std::vector<button> buttons{};
};

} // namespace model

#endif //NINJACLOWN_WORLD_HPP
