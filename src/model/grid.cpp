#include <algorithm>

#include "model/grid.hpp"

model::grid::grid(size_t width, size_t height) noexcept
{
	m_grid.resize(width);
	for (auto &column : m_grid) {
		column.reserve(height);
		std::generate_n(std::back_inserter(column), height, []() {
			return cell{cell_type::VOID};
		});
	}
}
