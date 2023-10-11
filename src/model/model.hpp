#ifndef NINJACLOWN_MODEL_MODEL_HPP
#define NINJACLOWN_MODEL_MODEL_HPP

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>

#include "bot/bot_dll.hpp"
#include "model/world.hpp"
#include "utils/loop_per_sec_limit.hpp"

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
	void async_load_dll(std::string dll_path) noexcept;
	void bot_start_level(ninja_api::nnj_api api) noexcept;
	void bot_end_level() noexcept;
	void bot_think() noexcept;
	void run();
	void stop() noexcept;
	bool is_running() noexcept;

	::model::world world{};

private:
	void do_run() noexcept;

	state::holder &m_state_holder;

	bot::bot_dll m_dll{};
	std::optional<std::string> m_tmp_dll_path{};
	std::atomic<bool> m_dll_await_load{};
	std::mutex m_async_load_dll{};

	std::optional<std::thread> m_thread{};
	std::atomic<thread_state> m_state{thread_state::waiting};
	std::mutex m_wait_mutex{};
	std::condition_variable m_cv{};

	utils::loop_per_sec_limit m_lps_limiter{15}; // updating the model 15 times per second at most
};
} // namespace model
#endif //NINJACLOWN_MODEL_MODEL_HPP
