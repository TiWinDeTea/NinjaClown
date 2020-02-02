#ifndef NINJACLOWN_GRID_ITERATOR_HPP
#define NINJACLOWN_GRID_ITERATOR_HPP

#include <iterator> // std::iterator, std::input_iterator_tag
#include <vector>

#include "cell.hpp"

namespace model {

class grid_iterator: public std::iterator<std::input_iterator_tag, int> {
public:
	grid_iterator(std::vector<std::vector<cell>> &grid, size_t start_x, size_t start_y, size_t target_x, size_t target_y)
	    : m_grid{grid}
	    , m_left_x{start_x}
	    , m_cur_x{start_x}
	    , m_cur_y{start_y}
	    , m_right_x{target_x}
	    , m_bottom_y{target_y} {}

	grid_iterator(const grid_iterator &other)
	    : m_grid{other.m_grid}
	    , m_left_x{other.m_left_x}
	    , m_cur_x{other.m_cur_x}
	    , m_cur_y{other.m_cur_y}
	    , m_right_x{other.m_right_x}
	    , m_bottom_y{other.m_bottom_y} {}

	grid_iterator(grid_iterator &&other) noexcept
	    : m_grid{other.m_grid}
	    , m_left_x{other.m_left_x}
	    , m_cur_x{other.m_cur_x}
	    , m_cur_y{other.m_cur_y}
	    , m_right_x{other.m_right_x}
	    , m_bottom_y{other.m_bottom_y} {}

	grid_iterator &operator++() {
		++m_cur_x;
		if (m_cur_x == m_right_x) {
			m_cur_x = m_left_x;
			++m_cur_y;
		}
		return *this;
	}

	const grid_iterator operator++(int) {
		grid_iterator tmp(*this);
		operator++();
		return tmp;
	}

	bool operator==(const grid_iterator &rhs) const {
		return m_cur_x == rhs.m_cur_x && m_cur_y == rhs.m_cur_y;
	}

	bool operator!=(const grid_iterator &rhs) const {
		return !operator==(rhs);
	}

	cell &operator*() {
		return m_grid[m_cur_x][m_cur_y];
	}

	grid_iterator end() {
		grid_iterator end{*this};
		end.m_cur_x = m_right_x;
		end.m_cur_y = m_bottom_y;
		return end;
	}

private:
	std::vector<std::vector<cell>> &m_grid;
	size_t m_left_x, m_cur_x, m_cur_y, m_right_x, m_bottom_y;
};

} // namespace model

#endif //NINJACLOWN_GRID_ITERATOR_HPP
