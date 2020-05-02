#ifndef NINJACLOWN_MODEL_MODEL_HPP
#define NINJACLOWN_MODEL_MODEL_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>

#include "bot/bot_dll.hpp"
#include "model/world.hpp"
#include "view/fps_limiter.hpp"

namespace state {
class holder;
}

namespace model {
class model {
	enum class thread_state {
		running,
		stopping,
		waiting,
	};

public:
	explicit model(state::holder *state_holder) noexcept;
	~model() noexcept;

	[[nodiscard]] bool load_dll(std::string dll_path) noexcept;
	void bot_start_level(bot::bot_api api) noexcept;
	void bot_end_level() noexcept;
	void bot_think() noexcept;
	void run();
	void stop() noexcept;

	::model::world world{};

private:
	void do_run() noexcept;

	state::holder &m_state_holder;

	bot::bot_dll m_dll{};

	std::optional<std::thread> m_thread{};
	std::atomic<thread_state> m_state{thread_state::waiting};
	std::mutex m_wait_mutex{};
	std::condition_variable m_cv{};

	view::fps_limiter m_fps_limiter{15};
};
} // namespace model
#endif //NINJACLOWN_MODEL_MODEL_HPP
