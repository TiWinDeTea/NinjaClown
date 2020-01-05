#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <spdlog/spdlog.h>

#include <imgui-SFML.h>
#include <imgui.h>

#include <imterm/terminal.hpp>

#include "program_state.hpp"
#include "terminal_commands.hpp"

#include "view/viewer.hpp"

namespace {
namespace cst {
	constexpr unsigned long window_width  = 800;
	constexpr unsigned long window_height = 450;
} // namespace cst
} // namespace

void view::viewer::run()
{
	m_thread  = std::make_unique<std::thread>(&viewer::do_run, this);
	m_running = true;
}

void view::viewer::stop() noexcept
{
	m_running = false;
	if (m_thread && m_thread->joinable()) {
		m_thread->join();
	}
}

void view::viewer::do_run() noexcept
{
	assert(program_state::global != nullptr);
	auto& terminal = program_state::global->terminal;

	terminal.set_width(cst::window_width);
    terminal.theme() = ImTerm::themes::cherry;
    terminal.log_level(ImTerm::message::severity::info);
    terminal.set_flags(ImGuiWindowFlags_NoTitleBar);
    terminal.disallow_x_resize();
    terminal.filter_hint() = "regex filter...";

	sf::Clock deltaClock;
	sf::Clock musicOffsetClock;

	sf::RenderWindow window({cst::window_width, cst::window_height}, "Ninja clown !");
	bool resized_once = false;
	window.setFramerateLimit(std::numeric_limits<unsigned int>::max());
	ImGui::SFML::Init(window);

	ImGuiIO &io    = ImGui::GetIO();
	io.IniFilename = nullptr;

	m_fps_limiter.start_now();

	auto step_bot = [&terminal]() {
        terminal_commands::argument_type arg{program_state::s_empty, terminal, {}};
        terminal_commands::update_world(arg);
	};
	bool auto_step_bot = false;

	while (window.isOpen() && m_running) {
		sf::Event event{};
		while (window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);

			if (event.type == sf::Event::Closed) {
				window.close();
			}
			else if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::F11) {
					program_state::global->term_on_display = !program_state::global->term_on_display;
				} else if (event.key.code == sf::Keyboard::F5) {
				    step_bot();
				} else if (event.key.code == sf::Keyboard::F4) {
				    auto_step_bot = !auto_step_bot;
				}
			}
			else if (event.type == sf::Event::Resized) {
				terminal.set_width(window.getSize().x);
				if (resized_once) {
					terminal.set_height(std::min(window.getSize().y, static_cast<unsigned>(terminal.get_size().y)));
				}
				resized_once = true;
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
			}
		}

		ImGui::SFML::Update(window, deltaClock.restart());

		if (program_state::global->term_on_display) {
			ImGui::SetNextWindowPos({0.f, 0.f}, ImGuiCond_Always);
			program_state::global->term_on_display = terminal.show();
			if (program_state::global->close_request) {
				window.close();
			}
		}

		if (auto_step_bot && !(current_frame() % 15)) {
		    step_bot();
		}

		window.clear();
		m_map.acquire()->print(window);
		for (const mob& mob : *m_mobs.acquire()) {
		    mob.print(window);
		}
		for (const auto& object : *m_objects.acquire()) {
		    object.print(window);
		}

		ImGui::SFML::Render(window);
		window.display();

		m_fps_limiter.wait();
	}
	if (window.isOpen()) {
		window.close();
	}
	ImGui::SFML::Shutdown();
}
