#ifndef NINJACLOWN_GRID_HPP
#define NINJACLOWN_GRID_HPP

#include <algorithm> // std::minmax
#include <cassert>
#include <iterator> // std::input_iterator_tag
#include <vector>

#include "model/cell.hpp"
#include "model/collision.hpp"
#include "model/grid_point.hpp"

namespace model {

struct cell_view {
	grid_point pos;
	cell_type &type;
	utils::optional<handle_t> &interaction_handle;
};

template <typename Ref>
class grid_iterator {
	using iterator_category = std::input_iterator_tag;
	using difference_type   = std::pair<int, int>;
	using reference         = Ref;

public:
	/**
   * @param start  top left corner
   * @param target bottom right corner
   */
	grid_iterator(std::vector<std::vector<cell>> &grid, grid_point start, grid_point target)
	    : m_grid{grid}
	    , m_start{start}
	    , m_current{start}
	    , m_end{target} { }

	grid_iterator(grid_iterator&&) = delete;

	grid_iterator(grid_iterator&) = delete;

	grid_iterator& operator=(grid_iterator&) = delete;

	grid_iterator& operator=(grid_iterator&&) = delete;

	grid_iterator &operator++() {
		++m_current.x;
		if (m_current.x == m_end.x) {
			m_current.x = m_start.x;
			++m_current.y;
		}
		return *this;
	}

	grid_iterator operator++(int) {
		grid_iterator tmp(*this);
		operator++();
		return tmp;
	}

	bool operator==(const grid_iterator &rhs) const {
		assert(&m_grid == &rhs.m_grid);
		return m_current == rhs.m_current;
	}

	bool operator!=(const grid_iterator &rhs) const {
		return !operator==(rhs);
	}

	reference operator*() {
		cell &cell = m_grid[m_current.x][m_current.y];
		return cell_view{m_current.x, m_current.y, cell.type, cell.interaction_handle};
	}

private:
	std::vector<std::vector<cell>> &m_grid;
	grid_point m_start;
	grid_point m_current;
	grid_point m_end;
};

class grid_view {
public:
	using value_type      = cell;
	using reference       = cell &;
	using const_reference = cell const &;
	using iterator        = grid_iterator<cell_view>;
	using const_iterator  = grid_iterator<const cell_view>;
	using difference_type = std::pair<std::int64_t, std::int64_t>;
	using size_type       = std::pair<std::size_t, std::size_t>;

	/**
	 * @param begin top left corner
	 * @param end   bottom right corner
	 */
	grid_view(std::vector<std::vector<cell>> &grid, grid_point begin, grid_point end)
	    : m_grid{grid}
	    , m_start{begin}
	    , m_end{end} { }

	grid_view(grid_view&&) = delete;

	grid_view(grid_view&) = delete;

	grid_view& operator=(grid_view&) = delete;

	grid_view& operator=(grid_view&&) = delete;

	iterator begin() {
		return grid_iterator<cell_view>{m_grid, m_start, m_end};
	}

	[[nodiscard]] const_iterator begin() const {
		return grid_iterator<const cell_view>{m_grid, m_start, m_end};
	}

	[[nodiscard]] const_iterator cbegin() const {
		return begin();
	}

	iterator end() {
		return grid_iterator<cell_view>{m_grid, {m_start.x, m_end.y}, grid_point::max()};
	}

	[[nodiscard]] const_iterator end() const {
		return grid_iterator<const cell_view>{m_grid, {m_start.x, m_end.y}, grid_point::max()};
	}

	[[nodiscard]] const_iterator cend() const {
		return end();
	}

private:
	std::vector<std::vector<cell>> &m_grid;
	grid_point m_start;
	grid_point m_end;
};

class grid {
public:
	grid() noexcept = default;

	grid(std::size_t width, std::size_t height) noexcept {
		resize(width, height);
	}

	void resize(std::size_t width, std::size_t height) {
		m_inner.resize(width);
		for (auto &column : m_inner) {
			column.resize(height);
		}
		m_width  = width;
		m_height = height;
	}

	[[nodiscard]] std::vector<cell> &operator[](std::size_t column) {
		return m_inner[column];
	}

	[[nodiscard]] const std::vector<cell> &operator[](std::size_t column) const {
		return m_inner[column];
	}

	[[nodiscard]] std::size_t width() const {
		return m_width;
	}

	[[nodiscard]] std::size_t height() const {
		return m_height;
	}

	/**
	 * @param begin top left corner
	 * @param end   bottom right corner
	 */
	[[nodiscard]] grid_view subgrid(grid_point begin, grid_point end) {
		assert(begin.x < end.x); // NOLINT
		assert(begin.y < end.y); // NOLINT
		return grid_view{m_inner,
                         {std::max<std::size_t>(begin.x, 0), std::max<std::size_t>(begin.y, 0)},
		                 {std::min(m_inner.size(), end.x),
		                  std::min(m_inner[0].size(), end.y)}};
	}

	[[nodiscard]] grid_view subgrid(const obb &box) {
		auto [min_x, max_x] = std::minmax({box.tl.x, box.br.x, box.bl.x, box.tr.x});
		auto [min_y, max_y] = std::minmax({box.tl.y, box.br.y, box.bl.y, box.tr.y});
		return subgrid({static_cast<std::size_t>(min_x), static_cast<std::size_t>(min_y)},
		               {static_cast<std::size_t>(max_x) + 1, static_cast<std::size_t>(max_y) + 1});
	}

	[[nodiscard]] grid_view radius(float center_x, float center_y, float radius) {
		auto start_x  = static_cast<std::size_t>(std::max(0.5f, center_x - radius));
		auto start_y  = static_cast<std::size_t>(std::max(0.5f, center_y - radius));
		auto target_x = static_cast<std::size_t>(std::min(m_inner.size() - 1, static_cast<std::size_t>(center_x + radius) + 1));
		auto target_y = static_cast<std::size_t>(std::min(m_inner[0].size() - 1, static_cast<std::size_t>(center_y + radius) + 1));

		return subgrid({start_x, start_y}, {target_x, target_y});
	}

private:
	std::size_t m_width{0};
	std::size_t m_height{0};
	std::vector<std::vector<cell>> m_inner{};
};

} // namespace model

#endif //NINJACLOWN_GRID_HPP
