#ifndef NINJACLOWN_VIEW_FPS_LIMITER_HPP
#define NINJACLOWN_VIEW_FPS_LIMITER_HPP

#include <chrono>
#include <thread>

#include "utils/synchronized.hpp"
#include "utils/spinlock.hpp"

namespace view {
class fps_limiter {
public:
	fps_limiter() noexcept {}

	// Do not call this method concurrently with itself or with .wait
	void start_now() noexcept
	{
		m_frame_count   = 0u;
		m_last_tick     = std::chrono::system_clock::now();
		*m_starting_time.acquire() = m_last_tick;
	}

	// Do not call this method concurrently with itself or with .start_now
	void wait() noexcept
	{
		if (m_refresh_frame_duration.exchange(false)) {
			m_frame_count   = 0;
			*m_starting_time.acquire() = m_last_tick;
            m_frame_duration = std::chrono::milliseconds(1000) / m_target_fps.load();
        }
		++m_frame_count;

		auto now                 = std::chrono::system_clock::now();
		auto last_frame_duration = m_last_tick - now;
		auto sleep_time          = m_frame_duration - last_frame_duration;
		if (sleep_time.count() > 0) {
			std::this_thread::sleep_for(sleep_time);
		}
		m_last_tick = std::chrono::system_clock::now();

		float target_fps  = static_cast<float>(m_target_fps.load());
		float current_fps = average_fps();

		if (current_fps - .05f > target_fps) {
			m_frame_duration += std::chrono::milliseconds(1);
		}
		else if (current_fps + .05f < target_fps) {
			m_frame_duration -= std::chrono::milliseconds(1);
		}
	}

	// can be called concurrently with any other method
	[[nodiscard]] unsigned int frame_count() const
	{
		return m_frame_count;
	}

	// can be called concurrently with any other method
	[[nodiscard]] float average_fps() const
	{
		using namespace std::chrono;
		auto display_duration = duration_cast<milliseconds>(system_clock::now() - *m_starting_time.acquire());
		return static_cast<float>(m_frame_count.load()) * 1000.f / display_duration.count();
	}

	// can be called concurrently with any other method
	[[nodiscard]] unsigned int target_fps() const
	{
		return m_target_fps;
	}

	// can be called concurrently with any other method
	void target_fps(unsigned int new_val) noexcept
	{
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
