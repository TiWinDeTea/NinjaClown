#ifndef NINJACLOWN_VIEW_FPS_LIMITER_HPP
#define NINJACLOWN_VIEW_FPS_LIMITER_HPP

#include <chrono>
#include <thread>

#include "utils/spinlock.hpp"
#include "utils/synchronized.hpp"

// TODO: move to utils

namespace view {
class fps_limiter {
public:
	fps_limiter() noexcept {};
	explicit fps_limiter(unsigned int target_fps) noexcept: m_target_fps{target_fps} {}

	// Do not call this method concurrently with itself or with .wait
	void start_now() noexcept;

	// Do not call this method concurrently with itself or with .start_now
	void wait() noexcept;

	// can be called concurrently with any other method
	[[nodiscard]] unsigned int frame_count() const {
		return m_frame_count;
	}

	// can be called concurrently with any other method
	[[nodiscard]] float average_fps() const;

	// can be called concurrently with any other method
	[[nodiscard]] unsigned int target_fps() const {
		return m_target_fps;
	}

	// can be called concurrently with any other method
	void target_fps(unsigned int new_val) noexcept {
		m_target_fps             = new_val;
		m_refresh_frame_duration = true;
	}

private:
	std::atomic_bool m_refresh_frame_duration{false};

	std::atomic_uint m_target_fps = 60;
	std::atomic_uint m_frame_count{0};
	std::chrono::milliseconds m_frame_duration{std::chrono::milliseconds(1000) / m_target_fps.load()};

	std::chrono::time_point<std::chrono::system_clock> m_last_tick{std::chrono::system_clock::now()};
	utils::synchronized<std::chrono::time_point<std::chrono::system_clock>, utils::spinlock> m_starting_time{m_last_tick};
};
} // namespace view

#endif //NINJACLOWN_VIEW_FPS_LIMITER_HPP
