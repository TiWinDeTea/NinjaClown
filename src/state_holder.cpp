#include "state_holder.hpp"

#include <spdlog/spdlog.h>
#include <filesystem>

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

state::holder::holder(const std::filesystem::path &config, const std::filesystem::path &autorun_script) noexcept
    : m_command_manager{build_commands_manager()}
    , resources{configure_resources(config)}
    , m_terminal{*this, "Terminal", 0, 0, m_command_manager}
    , m_model{this}
    , m_view{this}
    , m_adapter{this} {

	using view::viewer;

	properties.emplace("average_fps", property{&viewer::average_fps, m_view});

	properties.emplace("target_fps",
	                   property::proxy<unsigned int>::from_accessor<viewer>(m_view, &viewer::target_fps, &viewer::target_fps));

	properties.emplace("display_debug_data", property{&viewer::show_debug_data, m_view});

	m_command_manager->load_commands(resources);
	if (is_regular_file(autorun_script)) {
		std::ifstream autorun{autorun_script};
        std::string command_line;
		while (getline(autorun, command_line)) {
            m_terminal.execute(command_line);
		}
	}
}

void state::holder::run() noexcept {
	m_view.run();
}

void state::holder::wait() noexcept {
	m_view.wait();
}
