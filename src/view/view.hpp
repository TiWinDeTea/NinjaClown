#ifndef NINJACLOWN_VIEW_VIEW_HPP
#define NINJACLOWN_VIEW_VIEW_HPP

#include "adapter/facing_dir.hpp"
#include "utils/loop_per_sec_limit.hpp"

#include <SFML/System/Vector2.hpp>

#include <atomic>
#include <cassert>
#include <memory>
#include <thread>
#include <optional>

namespace sf {
class RenderWindow;
class Event;
}

namespace state {
class holder;
}

namespace utils::resources_type {
enum class tile_id;
}

namespace adapter {
struct view_handle;
namespace entity_edit {
	struct behaviour;
}
}


namespace view {

class game_viewer;
class map_viewer;
class map_editor;
class mob;
class object;

/**
 * Used mainly as a dispatcher
 */
class view {
	enum class window {
		game,
		menu,
		map_editor,
	};

public:
	~view();

	void exec(state::holder&);

	void stop() {
		m_running.clear();
	}

	void wait() {
		if (m_thread && m_thread->joinable()) {
			m_thread->join();
		}
	}

	bool has_map() const noexcept;

    game_viewer& game() noexcept {
        assert(m_game);
        return *m_game;
    }

    float average_fps() const noexcept {
        return m_fps_limiter.average_lps();
    }

    unsigned int target_fps() const noexcept {
        return m_fps_limiter.target_lps();
    }

    void target_fps(unsigned int target) noexcept {
		m_fps_limiter.target_lps(target);
    }

	void move_entity(adapter::view_handle handle, float new_x, float new_y) noexcept;
	void hide(adapter::view_handle handle) noexcept;
	void reveal(adapter::view_handle handle) noexcept;

	/**
	 * Used mainly when map editing
	 */
	void set_map(map_viewer&& map);
	void set_tile(unsigned int x, unsigned int y, utils::resources_type::tile_id id);
	adapter::view_handle add_mob(mob&& mob);
	adapter::view_handle add_object(object&& mob);
	void rotate_entity(adapter::view_handle handle, facing_direction::type dir) noexcept;
	void erase(adapter::view_handle) noexcept;
	void set_mob_kind(adapter::view_handle, adapter::entity_edit::behaviour) noexcept;

    std::atomic_bool show_debug_data{true};

private:
	map_viewer& map_viewer();
	void do_run(state::holder&);

	/**
	 * Polls through SFML events and manages them. Called while running
	 */
	void manage_events(sf::RenderWindow& window, state::holder&) noexcept;
	void manage_zoom(sf::Event, sf::RenderWindow&) noexcept;

	game_viewer* m_game{nullptr}; // allowing external access (!= nullptr while within *do_run*)
	map_editor* m_editor{nullptr}; // allowing external access (!= nullptr while within *do_run*)


	std::unique_ptr<std::thread> m_thread{};

    std::atomic_flag m_running{};

	utils::loop_per_sec_limit m_fps_limiter{};
	window m_show_state{window::map_editor}; // FIXME : devrait être window::menu
	bool m_showing_term{false};

	sf::Vector2i m_mouse_pos{};
	sf::Vector2u m_window_size{}; // lags behind one frame
	std::optional<sf::Vector2i> m_middle_click_pos{};
	std::optional<sf::Vector2i> m_right_click_pos{};

};
} // namespace view

#endif //NINJACLOWN_VIEW_HPP
