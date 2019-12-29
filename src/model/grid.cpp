#include <algorithm>

#include "model/grid.hpp"

model::grid::grid(size_t width, size_t height) noexcept
{
	m_grid.resize(width);
	for (auto &column : m_grid) {
		column.resize(height);
	}
}
