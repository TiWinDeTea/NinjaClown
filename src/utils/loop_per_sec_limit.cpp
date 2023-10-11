#include "loop_per_sec_limit.hpp"

void utils::loop_per_sec_limit::start_now() noexcept {
	m_loop_count               = 0u;
	m_last_tick                = std::chrono::system_clock::now();
	*m_starting_time.acquire() = m_last_tick;
}

void utils::loop_per_sec_limit::wait() noexcept {
	if (m_refresh_loop_duration.exchange(false)) {
		m_loop_count               = 0;
		*m_starting_time.acquire() = m_last_tick;
		m_loop_duration            = std::chrono::milliseconds(1000) / m_target_lps.load();
	}
	++m_loop_count;

	const auto now                = std::chrono::system_clock::now();
	const auto last_loop_duration = now - m_last_tick;
	const auto sleep_time         = m_loop_duration - last_loop_duration;
	if (sleep_time.count() > 0) {
		std::this_thread::sleep_for(sleep_time);
	}
	m_last_tick = std::chrono::system_clock::now();

	const auto target_lps  = static_cast<float>(m_target_lps.load());
	const auto current_lps = average_lps();

	if (current_lps - .05f > target_lps) {
		m_loop_duration += std::chrono::milliseconds(1);
	}
	else if (current_lps + .05f < target_lps) {
		if (m_loop_duration >= std::chrono::milliseconds(1)) {
			m_loop_duration -= std::chrono::milliseconds(1);
		}
	}
}

float utils::loop_per_sec_limit::average_lps() const {
	using namespace std::chrono; // NOLINT
	auto display_duration = static_cast<float>(duration_cast<milliseconds>(system_clock::now() - *m_starting_time.acquire()).count());
	return static_cast<float>(m_loop_count.load()) * 1000.f / display_duration;
}
