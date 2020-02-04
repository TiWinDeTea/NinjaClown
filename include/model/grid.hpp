#ifndef NINJACLOWN_GRID_HPP
#define NINJACLOWN_GRID_HPP

#include <iterator> // std::iterator, std::input_iterator_tag
#include <vector>
#include <algorithm>

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

	friend class grid_view;
};

class grid_view {
public:
	using value_type      = cell;
	using reference       = cell &;
	using const_reference = cell const &;
	using iterator        = grid_iterator;
	using const_iterator  = const grid_iterator;
	using difference_type = long;
	using size_type       = size_t;

	grid_view(std::vector<std::vector<cell>> &grid, size_t left_x, size_t top_y, size_t right_x, size_t bottom_y)
	    : m_grid{grid}
	    , m_left_x{left_x}
	    , m_top_y{top_y}
	    , m_right_x{right_x}
	    , m_bottom_y{bottom_y} {}

	grid_view(const grid_view &other) = default;

	grid_view(grid_view &&other) noexcept
	    : m_grid{other.m_grid}
	    , m_left_x{other.m_left_x}
	    , m_top_y{other.m_top_y}
	    , m_right_x{other.m_right_x}
	    , m_bottom_y{other.m_bottom_y} {}

	grid_iterator begin() {
		return grid_iterator{m_grid, m_left_x, m_top_y, m_right_x, m_bottom_y};
	}

	const grid_iterator begin() const {
		return grid_iterator{m_grid, m_left_x, m_top_y, m_right_x, m_bottom_y};
	}

	const grid_iterator cbegin() const {
		return begin();
	}

	grid_iterator end() {
		return grid_iterator{m_grid, m_right_x, m_bottom_y, m_right_x, m_bottom_y};
	}

	const grid_iterator end() const {
		return grid_iterator{m_grid, m_right_x, m_bottom_y, m_right_x, m_bottom_y};
	}

	const grid_iterator cend() const {
		return end();
	}

private:
	std::vector<std::vector<cell>> &m_grid;
	size_t m_left_x, m_top_y, m_right_x, m_bottom_y;
};

class grid_t {
public:
	grid_t() noexcept = default;

	grid_t(size_t width, size_t height) noexcept {
		resize(width, height);
	}

	void resize(size_t width, size_t height) {
		m_inner.resize(width);
		for (auto &column : m_inner) {
			column.resize(height);
		}
	}

	std::vector<cell> &operator[](size_t column) {
		return m_inner[column];
	}

	const std::vector<cell> &operator[](size_t column) const {
		return m_inner[column];
	}

	size_t size() const {
		return m_inner.size();
	}

	grid_view subgrid(size_t left_x, size_t top_y, size_t right_x, size_t bottom_y) {
		return grid_view{m_inner, left_x, top_y, right_x, bottom_y};
	}

	grid_view radius(float center_x, float center_y, float radius) {
		auto start_x  = static_cast<size_t>(std::max(0.5f, center_x - radius));
		auto start_y  = static_cast<size_t>(std::max(0.5f, center_y - radius));
		auto target_x = std::min(m_inner.size() - 1, static_cast<size_t>(center_x + radius));
		auto target_y = std::min(m_inner[0].size() - 1, static_cast<size_t>(center_y + radius));

		return subgrid(start_x, start_y, target_x, target_y);
	}

private:
	std::vector<std::vector<cell>> m_inner{};
};

} // namespace model

#endif //NINJACLOWN_GRID_HPP
