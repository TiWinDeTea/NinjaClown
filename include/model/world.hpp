#ifndef NINJACLOWN_WORLD_HPP
#define NINJACLOWN_WORLD_HPP

#include <string>
#include <vector>

#include "cell.hpp"
#include "interaction.hpp"
#include "interactables/button.hpp"

namespace model {
struct world {
	world(size_t width, size_t height) noexcept;
	explicit world(const std::string &map_path);

	void update();

private:
	std::vector<std::vector<cell>> m_grid{};

	std::vector<interaction> m_interactions{};
	std::vector<button> m_buttons{};
};
} // namespace model

#endif //NINJACLOWN_WORLD_HPP
