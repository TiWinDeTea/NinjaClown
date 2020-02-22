#include "state_holder.hpp"

#include <spdlog/spdlog.h>

namespace {
utils::resource_manager configure_resources(const std::filesystem::path &config) {
	utils::resource_manager rm{};
	if (!rm.load_config(config)) {
		spdlog::critical("Failed to load resources from {}", config.generic_string());
	}
	return rm;
}

std::shared_ptr<terminal_commands> build_commands_manager() {
	auto manager = std::make_shared<terminal_commands>();
    spdlog::default_logger()->sinks().push_back(manager);
	return manager;
}
} // namespace

state::holder::holder(const std::filesystem::path &config) noexcept
    : m_command_manager{build_commands_manager()}
    , resources{configure_resources(config)}
    , m_terminal{*this, "Terminal", 0, 0, m_command_manager}
    , m_model{this}
    , m_view{this}
    , m_adapter{this} {
	m_command_manager->load_commands(resources);
}

void state::holder::run() noexcept {
	m_view.run();
}

void state::holder::wait() noexcept {
	m_view.wait();
}
