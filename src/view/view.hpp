#ifndef NINJACLOWN_VIEW_VIEW_HPP
#define NINJACLOWN_VIEW_VIEW_HPP

#include "utils/loop_per_sec_limit.hpp"

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

    std::atomic_bool show_debug_data{true};

private:
	void do_run(state::holder&);

	/**
	 * Polls through SFML events and manages them. Called while running
	 */
	void manage_events(sf::RenderWindow& window, state::holder&) noexcept;

	game_viewer* m_game{nullptr}; // allowing external access (data within *do_run*)


	std::unique_ptr<std::thread> m_thread{};

    std::atomic_flag m_running{};

	utils::loop_per_sec_limit m_fps_limiter{};
	window m_show_state{window::game}; // FIXME : devrait être window::menu
	bool m_showing_term{false};
};
} // namespace view

#endif //NINJACLOWN_VIEW_HPP
