#ifndef NINJACLOWN_VIEW_FPS_LIMITER_HPP
#define NINJACLOWN_VIEW_FPS_LIMITER_HPP

#include <chrono>
#include <thread>

#include "utils/spinlock.hpp"
#include "utils/synchronized.hpp"

// TODO: move to utils

namespace utils {
/**
 * Limits the amount of time a loop is updated per second
 */
class loop_per_sec_limit {
public:
	loop_per_sec_limit() noexcept {};
	explicit loop_per_sec_limit(unsigned int target_lps) noexcept: m_target_lps{target_lps} {}

	// Do not call this method concurrently with itself or with .wait
	void start_now() noexcept;

	// Do not call this method concurrently with itself or with .start_now
	void wait() noexcept;

	// can be called concurrently with any other method
	[[nodiscard]] unsigned int loop_count() const {
		return m_loop_count;
	}

	// can be called concurrently with any other method
	[[nodiscard]] float average_lps() const;

	// can be called concurrently with any other method
	[[nodiscard]] unsigned int target_lps() const {
		return m_target_lps;
	}

	// can be called concurrently with any other method
	void target_lps(unsigned int new_val) noexcept {
		m_target_lps             = new_val;
		m_refresh_loop_duration  = true;
	}

private:
	std::atomic_bool m_refresh_loop_duration{false};

	std::atomic_uint m_target_lps = 60;
	std::atomic_uint m_loop_count{0};
	std::chrono::milliseconds m_loop_duration{std::chrono::milliseconds(1000) / m_target_lps.load()};

	std::chrono::time_point<std::chrono::system_clock> m_last_tick{std::chrono::system_clock::now()};
	utils::synchronized<std::chrono::time_point<std::chrono::system_clock>, utils::spinlock> m_starting_time{m_last_tick};
};
} // namespace view

#endif //NINJACLOWN_VIEW_FPS_LIMITER_HPP
