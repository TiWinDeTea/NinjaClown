#ifndef NINJACLOWN_VIEW_VIEW_HPP
#define NINJACLOWN_VIEW_VIEW_HPP

#include "view/fps_limiter.hpp"

#include <cassert>

#include <atomic>
#include <memory>
#include <thread>

namespace sf {
class RenderWindow;
}

namespace state {
class holder;
}

namespace view {

class game_viewer;
class menu;

class view {
	enum class window {
		game,
		menu,
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
        return m_fps_limiter.average_fps();
    }

    unsigned int target_fps() const noexcept {
        return m_fps_limiter.target_fps();
    }

    void target_fps(unsigned int target) noexcept {
        m_fps_limiter.target_fps(target);
    }

    std::atomic_bool show_debug_data{true};

private:
	void do_run(state::holder&);

	/**
	 * Polls through SFML events and manages them. Called while running
	 */
	void manage_events(sf::RenderWindow& window, unsigned int terminal_height) noexcept;

	::view::menu* m_menu{nullptr}; // allowing external access (data within *do_run*)
	game_viewer* m_game{nullptr}; // allowing external access (data within *do_run*)

	std::unique_ptr<std::thread> m_thread{};

    std::atomic_flag m_running{};

	fps_limiter m_fps_limiter{};
	window m_showing{window::game}; // FIXME : devrait être window::menu
	bool m_showing_term{false};
};
} // namespace view

#endif //NINJACLOWN_VIEW_HPP
