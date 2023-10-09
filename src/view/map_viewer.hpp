#ifndef NINJACLOWN_VIEW_MAP_VIEWER_HPP
#define NINJACLOWN_VIEW_MAP_VIEWER_HPP

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

#include "utils/synchronized.hpp"
#include "utils/spinlock.hpp"

#include "view/map.hpp"
#include "view/overmap_collection.hpp"

namespace sf {
class RenderWindow;
class Sprite;
class RectangleShape;
}

namespace adapter {
struct view_handle;
}
namespace state {
class holder;
}


#include <SFML/Graphics/RenderWindow.hpp>

namespace view {
class map_viewer {

public:
	explicit map_viewer(state::holder& state) noexcept;
    map_viewer(map_viewer&&) noexcept = default;
    map_viewer& operator=(map_viewer&&) noexcept = default;

	void set_render_window(sf::RenderWindow& window) noexcept {
		m_window = &window;
		m_viewport = window.getView().getViewport(); // TODO REMOVE THIS STATEMENT
	}

	// viewport of the underlying map
	void viewport(sf::FloatRect viewport) noexcept {
		m_viewport = viewport;
	}

	[[nodiscard]] sf::FloatRect viewport() const noexcept {
		return m_viewport;
	}

	// viewport of this object
	void set_drawing_region_viewport(sf::FloatRect viewport) noexcept {
		m_drawing_region_viewport = viewport;
	}

	/**
	 * Prints the map plus some tooltip infos
	 */
	void print(bool show_debug_data);
	/**
	 * Prints some tooltip infos
	 */
	void print_tile_info(bool show_debug_data);

    // converts world grid coords to on-screen coords
    sf::Vector2f to_screen_coords(float x, float y) const noexcept;

    // coords within the viewport
    sf::Vector2f get_mouse_pos() const noexcept;

    void draw(sf::Sprite&);

    void draw(sf::RectangleShape&);

    void highlight_tile(sf::Vector2i tile_coord);

	[[nodiscard]] unsigned int current_frame() const noexcept {
		return m_current_frame;
	}

    void reload_sprites();

	std::chrono::system_clock::time_point starting_time() {
		return m_starting_time;
	}

    [[nodiscard]] bool is_filled() const noexcept {
		return !m_map.acquire()->empty();
    }

	auto acquire_overmap() {
		return m_overmap.acquire();
	}

private:

	void set_map(std::vector<std::vector<map::cell>>&& new_map) {
        auto map = m_map.acquire();
        map->set(std::move(new_map));
        m_level_size = map->level_size();
	}


    state::holder* m_state{nullptr};
	sf::RenderWindow* m_window{nullptr};

    std::chrono::system_clock::time_point m_starting_time{std::chrono::system_clock::now()};

    utils::synchronized_moveable<overmap_collection> m_overmap{};
    utils::synchronized_moveable<map, utils::spinlock> m_map{};
	sf::Vector2u m_level_size{};

	sf::FloatRect m_viewport{};
	sf::FloatRect m_drawing_region_viewport{};

	unsigned int m_current_frame{};


    friend class adapter::adapter;
};
}  // namespace view

#endif //NINJACLOWN_VIEW_MAP_VIEWER_HPP
