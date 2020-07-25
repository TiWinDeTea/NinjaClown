#ifndef NINJACLOWN_VIEW_VIEWER_HPP
#define NINJACLOWN_VIEW_VIEWER_HPP

#include <atomic>
#include <memory>
#include <unordered_set>

#include <SFML/Graphics/Rect.hpp>

#include "utils/synchronized.hpp"

#include "view/event_inspector.hpp"
#include "view/file_explorer.hpp"
#include "view/fps_limiter.hpp"
#include "view/map.hpp"
#include "view/mob.hpp"
#include "view/object.hpp"
#include "view/overmap_collection.hpp"

namespace sf {
class RenderWindow;

template <typename>
class Vector2;
typedef Vector2<float> Vector2f;
typedef Vector2<int> Vector2i;
}

namespace adapter {
struct view_handle;
}
namespace state {
class holder;
}

namespace view {

// todo: nettoyer cette classe
class viewer {
public:
	explicit viewer(state::holder *state_holder) noexcept;

	~viewer() {
		stop();
	}

	void run();

	void stop() noexcept;

	void wait();

	// can be called concurrently
	void target_fps(unsigned int fps) noexcept {
		m_fps_limiter.target_fps(fps);
	}

	[[nodiscard]] std::pair<std::size_t, std::size_t> level_size() const noexcept {
		return m_level_size;
	}

	// can be called concurrently
	[[nodiscard]] unsigned int target_fps() const noexcept {
		return m_fps_limiter.target_fps();
	}

	// can be called concurrently
	[[nodiscard]] float average_fps() const noexcept {
		return m_fps_limiter.average_fps();
	}

	[[nodiscard]] unsigned int current_frame() const noexcept {
		return m_fps_limiter.frame_count();
	}

	void reload_sprites();

	utils::synchronized<overmap_collection>::acquired_t acquire_overmap() noexcept;

	utils::synchronized<view::map, utils::spinlock>::acquired_t acquire_map() noexcept;

	void set_map(std::vector<std::vector<map::cell>> &&new_map) noexcept;

	// converts world grid coords to on-screen coords
	sf::Vector2f to_screen_coords(float x, float y) const noexcept;

	// coords within the viewport
	sf::Vector2f get_mouse_pos() const noexcept;

	const std::chrono::system_clock::time_point starting_time{std::chrono::system_clock::now()};

	std::atomic_bool show_debug_data{true};
	bool close_requested{false};

	sf::RenderWindow *window;

private:

	void do_run() noexcept;

	void show_menu_window(struct viewer_display_state&) noexcept;

	// converts sfml events coords to viewport coords
	sf::Vector2f to_viewport_coord(const sf::Vector2f &coords) const noexcept;
	sf::Vector2f to_viewport_coord(const sf::Vector2i &coords) const noexcept;

	state::holder &m_state_holder;

	utils::synchronized<overmap_collection> m_overmap;
	utils::synchronized<view::map, utils::spinlock> m_map{};
	std::pair<std::size_t, std::size_t> m_level_size{};

	std::unique_ptr<std::thread> m_thread{};
	std::atomic_bool m_running{false};

	sf::FloatRect m_viewport{};

	fps_limiter m_fps_limiter{};

	friend bool view::inspect_event(viewer &viewer, const sf::Event &event, struct viewer_display_state &state);
};
} // namespace view

#endif //NINJACLOWN_VIEW_VIEWER_HPP
