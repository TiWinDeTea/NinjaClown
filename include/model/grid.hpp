#ifndef NINJACLOWN_GRID_HPP
#define NINJACLOWN_GRID_HPP

#include <algorithm> // std::minmax
#include <cassert>
#include <iterator> // std::input_iterator_tag
#include <vector>

#include "cell.hpp"
#include "collision.hpp"

namespace model {

template <typename T>
class grid_iterator {
	using iterator_category = std::input_iterator_tag;
	using value_type        = T;
	using difference_type   = std::pair<int, int>;
	using pointer           = T *;
	using reference         = T &;

public:
	grid_iterator(std::vector<std::vector<cell>> &grid, size_t start_x, size_t start_y, size_t target_x, size_t target_y)
	    : m_grid{grid}
	    , m_left_x{start_x}
	    , m_cur_x{start_x}
	    , m_cur_y{start_y}
	    , m_right_x{target_x}
	    , m_bottom_y{target_y} {}

	grid_iterator &operator++() {
		++m_cur_x;
		if (m_cur_x == m_right_x) {
			m_cur_x = m_left_x;
			++m_cur_y;
		}
		return *this;
	}

	grid_iterator operator++(int) {
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

	reference operator*() {
		return m_grid[m_cur_x][m_cur_y];
	}

private:
	std::vector<std::vector<cell>> &m_grid;
	size_t m_left_x, m_cur_x, m_cur_y, m_right_x, m_bottom_y;
};

class grid_view {
public:
	using value_type      = cell;
	using reference       = cell &;
	using const_reference = cell const &;
	using iterator        = grid_iterator<cell>;
	using const_iterator  = grid_iterator<const cell>;
	using difference_type = std::pair<std::int64_t, std::int64_t>;
	using size_type       = std::pair<size_t, size_t>;

	grid_view(std::vector<std::vector<cell>> &grid, size_t left_x, size_t top_y, size_t right_x, size_t bottom_y)
	    : m_grid{grid}
	    , m_left_x{left_x}
	    , m_top_y{top_y}
	    , m_right_x{right_x}
	    , m_bottom_y{bottom_y} {}

	iterator begin() {
		return grid_iterator<cell>{m_grid, m_left_x, m_top_y, m_right_x, m_bottom_y};
	}

    [[nodiscard]]
    const_iterator begin() const {
		return grid_iterator<const cell>{m_grid, m_left_x, m_top_y, m_right_x, m_bottom_y};
	}

    [[nodiscard]]
    const_iterator cbegin() const {
		return begin();
	}

	iterator end() {
		return grid_iterator<cell>{m_grid, m_left_x, m_bottom_y, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};
	}

	[[nodiscard]]
	const_iterator end() const {
		return grid_iterator<const cell>{m_grid, m_right_x, m_bottom_y, m_right_x, m_bottom_y};
	}

    [[nodiscard]]
    const_iterator cend() const {
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
		m_width = width;
		m_height = height;
	}

	[[nodiscard]]
	std::vector<cell> &operator[](size_t column) {
		return m_inner[column];
	}

    [[nodiscard]]
	const std::vector<cell> &operator[](size_t column) const {
		return m_inner[column];
	}

    [[nodiscard]]
    size_t width() const {
        return m_width;
    }

    [[nodiscard]]
    size_t height() const {
        return m_height;
    }

	[[nodiscard]]
	grid_view subgrid(size_t left_x, size_t top_y, size_t right_x, size_t bottom_y) {
		assert(left_x < right_x);
		assert(top_y < bottom_y);
		return grid_view{m_inner, left_x, top_y, std::min(m_inner.size(), right_x), std::min(m_inner[0].size(), bottom_y)};
	}

    [[nodiscard]]
	grid_view subgrid(const bounding_box &box) {
		auto [min_x, max_x] = std::minmax({box.tl.x, box.br.x, box.bl.x, box.tr.x});
		auto [min_y, max_y] = std::minmax({box.tl.y, box.br.y, box.bl.y, box.tr.y});
		return subgrid(static_cast<size_t>(min_x), static_cast<size_t>(min_y), static_cast<size_t>(max_x) + 1,
		               static_cast<size_t>(max_y) + 1);
	}

    [[nodiscard]]
	grid_view radius(float center_x, float center_y, float radius) {
		auto start_x  = static_cast<size_t>(std::max(0.5f, center_x - radius));
		auto start_y  = static_cast<size_t>(std::max(0.5f, center_y - radius));
		auto target_x = std::min(m_inner.size() - 1, static_cast<size_t>(center_x + radius) + 1);
		auto target_y = std::min(m_inner[0].size() - 1, static_cast<size_t>(center_y + radius) + 1);

		return subgrid(start_x, start_y, target_x, target_y);
	}

private:
    size_t m_width{0};
    size_t m_height{0};
	std::vector<std::vector<cell>> m_inner{};
};

} // namespace model

#endif //NINJACLOWN_GRID_HPP
