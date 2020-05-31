#include "view/fps_limiter.hpp"

void view::fps_limiter::start_now() noexcept {
    m_frame_count              = 0u;
    m_last_tick                = std::chrono::system_clock::now();
    *m_starting_time.acquire() = m_last_tick;
}

void view::fps_limiter::wait() noexcept {
    if (m_refresh_frame_duration.exchange(false)) {
        m_frame_count              = 0;
        *m_starting_time.acquire() = m_last_tick;
        m_frame_duration           = std::chrono::milliseconds(1000) / m_target_fps.load();
    }
    ++m_frame_count;

    auto now                 = std::chrono::system_clock::now();
    auto last_frame_duration = now - m_last_tick;
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
        if (m_frame_duration >= std::chrono::milliseconds(1)) {
            m_frame_duration -= std::chrono::milliseconds(1);
        }
    }
}

float view::fps_limiter::average_fps() const {
    using namespace std::chrono;
    auto display_duration = duration_cast<milliseconds>(system_clock::now() - *m_starting_time.acquire());
    return static_cast<float>(m_frame_count.load()) * 1000.f / display_duration.count();
}



