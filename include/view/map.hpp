#ifndef NINJACLOWN_VIEW_MAP_HPP
#define NINJACLOWN_VIEW_MAP_HPP

#include <vector>

namespace adapter {
class adapter;
}

namespace sf {
class RenderWindow;
}

namespace utils {
class resource_manager;
}

namespace view {
class viewer;

class map {
public:
	enum class cell {
		iron_tile,
		concrete_tile,
		abyss,
		target_tile,
	};

	void set(std::vector<std::vector<cell>> &&cells) {
		m_cells = std::move(cells);
	}

	[[nodiscard]] std::pair<std::size_t, std::size_t> level_size() const noexcept {
		if (m_cells.empty()) {
			return {0u, 0u};
		}
		return {m_cells.size(), m_cells.front().size()};
	}

	void print(view::viewer& view, utils::resource_manager& resources) const noexcept;

	void highlight_tile(view::viewer& view, size_t x, size_t y, utils::resource_manager& resources) const noexcept;

	void frame_tile(view::viewer& view, size_t x, size_t y, utils::resource_manager& resources) const noexcept;

private:
	std::vector<std::vector<cell>> m_cells;

	friend class adapter::adapter;
};
} // namespace view

#endif //NINJACLOWN_VIEW_MAP_HPP
