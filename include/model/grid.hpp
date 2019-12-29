#ifndef NINJACLOWN_GRID_HPP
#define NINJACLOWN_GRID_HPP

#include <vector>

#include "model/cell.hpp"

namespace model {
class grid {
public:
	grid(size_t width, size_t height) noexcept;

private:
	std::vector<std::vector<cell>> m_grid{};
};
} // namespace model

#endif //NINJACLOWN_GRID_HPP
