#ifndef NINJACLOWN_VIEW_GAME_VIEWER_HPP
#define NINJACLOWN_VIEW_GAME_VIEWER_HPP

#include "map_viewer.hpp"
#include "game_menu.hpp"

#include "terminal_commands.hpp"

namespace sf {
class RenderWindow;
class Event;
} // namespace sf

namespace state {
class holder;
}

namespace adapter {
struct view_handle;
}

namespace view {
class game_viewer {
public:
	explicit game_viewer(sf::RenderWindow &window, state::holder &state) noexcept;

	void event(const sf::Event &ev);

	void set_map(map_viewer &&map_viewer) {
		m_map = std::move(map_viewer);
		m_map.set_render_window(m_window);
	}

	void pause() noexcept;

	void resume() noexcept;

	void restart() noexcept;

	/**
	 * Returns false if return to main menu was requested.
	 */
	bool show(bool show_debug_data);

	void reload_sprites() {
		m_map.reload_sprites();
	}

	[[nodiscard]] bool has_map() const noexcept {
		return m_map.is_filled();
	}

    void move_entity(const adapter::view_handle& handle, float new_x, float new_y) {
        m_map.acquire_overmap()->move_entity(handle, new_x, new_y);
    }

    void rotate_entity(const adapter::view_handle& handle, ::view::facing_direction::type value) {
        m_map.acquire_overmap()->rotate_entity(handle, value);
    }

    void reveal(const adapter::view_handle& handle) {
        m_map.acquire_overmap()->reveal(handle);
    }

    void hide(const adapter::view_handle& handle) {
        m_map.acquire_overmap()->hide(handle);
    }

	/**
	 * Displays the rightmost bar (play, pause, step, ... buttons).
	 */
	void show_rightmost_bar() noexcept;

	/**
	 * Shows the escape menu.
	 */
	void display_menu() noexcept;

private:
	sf::RenderWindow &m_window;
    state::holder& m_state;
    map_viewer m_map;
	game_menu m_menu;
	bool m_showing_menu{false};
	bool m_stay_in_game{true};

	terminal_commands::argument_type m_fake_arg; //! Mostly valid argument passed to the terminal by the view, instead of by ImTerm

	// events related
	sf::Vector2i m_mouse_pos{};
	sf::Vector2u m_window_size{}; // lags behind one frame
	std::optional<sf::Vector2i> m_left_click_pos{};
	std::optional<sf::Vector2i> m_right_click_pos{};

	bool m_autostep_bot{false};
};
} // namespace view

#endif //NINJACLOWN_VIEW_GAME_VIEWER_HPP
