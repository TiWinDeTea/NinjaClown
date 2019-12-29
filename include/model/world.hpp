#ifndef NINJACLOWN_WORLD_HPP
#define NINJACLOWN_WORLD_HPP

#include "model/grid.hpp"

namespace model {
class world {
public:
	world(size_t width, size_t height) noexcept
	    : m_grid{width, height}
	{
	}

private:
	grid m_grid;
};
} // namespace model

#endif //NINJACLOWN_WORLD_HPP
