#ifndef NINJACLOWN_VIEW_MAP_HPP
#define NINJACLOWN_VIEW_MAP_HPP

#include <vector>

namespace adapter {
class adapter;
}

namespace sf {
class RenderWindow;

template <typename>
class Vector2;
typedef Vector2<unsigned int> Vector2u;
}

namespace utils {
class resource_manager;
}

namespace view {
class map_viewer;

class map {
public:
	enum class cell {
		iron_tile,
		concrete_tile,
		abyss,
	};

	void set(std::vector<std::vector<cell>> &&cells) {
		m_cells = std::move(cells);
	}

	[[nodiscard]] sf::Vector2u level_size() const noexcept;

	void print(map_viewer& view) const noexcept;

	void highlight_tile(map_viewer& view, std::size_t x, std::size_t y) const noexcept;

	void frame_tile(map_viewer& view, std::size_t x, std::size_t y) const noexcept;

	[[nodiscard]] bool empty() const noexcept {
		return m_cells.empty();
	}

	void set_tile(unsigned int x, unsigned int y, cell c) noexcept {
		m_cells[x][y] = c;
	}

private:
	std::vector<std::vector<cell>> m_cells;

	friend class adapter::adapter;
};
} // namespace view

#endif //NINJACLOWN_VIEW_MAP_HPP
