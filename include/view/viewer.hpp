#ifndef NINJACLOWN_VIEW_VIEWER_HPP
#define NINJACLOWN_VIEW_VIEWER_HPP

#include "utils/synchronized.hpp"

#include "view/map.hpp"
#include "view/fps_limiter.hpp"

namespace view {
class viewer {
public:
    ~viewer() {
        stop();
    }

    void run();

    void stop() noexcept;

    void wait() {
        if (m_thread && m_thread->joinable()) {
            m_thread->join();
        }
    }

    // can be called concurrently
    void target_fps(unsigned int fps) noexcept {
        m_fps_limiter.target_fps(fps);
    }

    [[nodiscard]] std::pair<std::size_t, std::size_t> level_size() const noexcept {
        return m_map.level_size();
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

private:

    void do_run() noexcept;

    view::map m_map{};
    std::unique_ptr<std::thread> m_thread{};
    std::atomic_bool m_running{false};

    fps_limiter m_fps_limiter{};
};
}

#endif //NINJACLOWN_VIEW_VIEWER_HPP
