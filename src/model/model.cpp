#include "model/model.hpp"
#include "state_holder.hpp"
#include <spdlog/spdlog.h>

model::model::model(state::holder *state_holder) noexcept: m_state_holder{*state_holder} {};

[[nodiscard]] bool model::model::load_dll(std::string dll_path) noexcept {
	return m_dll.load(std::move(dll_path));
}

void model::model::bot_init(bot::bot_api api) noexcept {
	api.ninja_descriptor = this;
	m_dll.bot_init(api);
}

void model::model::bot_think() noexcept {
	m_dll.bot_think();
}

void model::model::run() {
	m_running = true;
	m_thread.emplace(&model::do_run, this);
}

void model::model::stop() noexcept {
	m_running = false;
	if (m_thread && m_thread->joinable()) {
		m_thread->join();
	}
}

void model::model::do_run() noexcept {
	m_fps_limiter.start_now();
	while (m_running) {
		bot_think();
		world.update(state::access<model>::adapter(m_state_holder));
		m_fps_limiter.wait();
	}
}
