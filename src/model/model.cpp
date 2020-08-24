#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"

model::model::model(state::holder *state_holder) noexcept
    : m_state_holder{*state_holder} {
	m_thread.emplace(&model::do_run, this);
}

model::model::~model() noexcept {
	if (m_state.exchange(thread_state::stopping) == thread_state::waiting) {
		m_cv.notify_one();
	}

	if (m_thread && m_thread->joinable()) {
		m_thread->join();
	}
}

[[nodiscard]] bool model::model::load_dll(std::string dll_path) noexcept {
	if (m_dll.load(m_state_holder.resources(), std::move(dll_path))) {
		m_dll.bot_init();
		return true;
	}

	return false;
}

void model::model::async_load_dll(std::string dll_path) noexcept {
	std::scoped_lock sl{m_async_load_dll};
	m_tmp_dll_path = {dll_path};
	m_dll_await_load.store(true);
}

void model::model::bot_start_level(ninja_api::nnj_api api) noexcept {
	api.ninja_descriptor = &m_state_holder;
	m_dll.bot_start_level(api);
}

void model::model::bot_end_level() noexcept {
	m_dll.bot_end_level();
}

void model::model::bot_think() noexcept {
	m_dll.bot_think();
}

void model::model::run() {
	if (m_state == thread_state::waiting && m_dll) {
		m_state = thread_state::running;
		m_cv.notify_one();
	}
}

void model::model::stop() noexcept {
	if (m_state == thread_state::running) {
		m_state = thread_state::waiting;
	}
}

bool model::model::is_running() noexcept {
	return m_state.load() == thread_state::running;
}

void model::model::do_run() noexcept {
	{
		std::unique_lock ul{m_wait_mutex};
		m_cv.wait(ul, [this]() {
			return m_state != thread_state::waiting;
		});
	}
	m_fps_limiter.start_now();

	while (m_state != thread_state::stopping) {
		if (m_dll_await_load.load()) {
			std::scoped_lock sl{m_async_load_dll};
			if (m_dll.load(m_state_holder.resources(), std::move(*m_tmp_dll_path))) {
				m_dll.bot_init();
			}
			m_tmp_dll_path = {};
			m_dll_await_load.store(false);
		}

		bot_think();

		adapter::adapter &adapter = state::access<model>::adapter(m_state_holder);
		adapter.clear_cells_changed_since_last_update();
		adapter.clear_entities_changed_since_last_update();
		world.update(adapter);

		m_fps_limiter.wait();

		if (m_state == thread_state::waiting) {
			std::unique_lock ul{m_wait_mutex};
			m_cv.wait(ul, [this]() {
				return m_state != thread_state::waiting;
			});
			m_fps_limiter.start_now();
		}
	}
}
