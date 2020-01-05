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

	/**********************<TEST>******************************/
	std::vector<std::vector<map::cell>> cells;
	cells.resize(12);
	for (auto &v : cells) {
		v.resize(5, map::cell::concrete_tile);
	}
	for (auto &cell : cells[4]) {
		cell = map::cell::abyss;
	}
    for (auto it = std::next(cells[1].begin()) ; it != std::prev(cells[1].end()) ; ++it) {
        *it = map::cell::iron_tile;
    }
    for (auto it = std::next(cells[2].begin()) ; it != std::prev(cells[2].end()) ; ++it) {
        *it = map::cell::iron_tile;
    }
    for (auto it = std::next(cells[3].begin()) ; it != std::prev(cells[3].end()) ; ++it) {
        *it = map::cell::iron_tile;
    }
	for (auto &cell : cells) {
		cell[0] = map::cell::abyss;
		cell[4] = map::cell::abyss;
	}
	for (auto &cell : cells[0]) {
		cell = map::cell::abyss;
	}
	for (auto &cell : cells[11]) {
		cell = map::cell::abyss;
	}
	m_map.set(std::move(cells));
    /**********************</TEST>*****************************/

	sf::Clock deltaClock;
	sf::Clock musicOffsetClock;

	sf::RenderWindow window({cst::window_width, cst::window_height}, "Ninja clown !");
	bool resized_once = false;
	window.setFramerateLimit(std::numeric_limits<unsigned int>::max());
	ImGui::SFML::Init(window);

	ImGuiIO &io    = ImGui::GetIO();
	io.IniFilename = nullptr;

	m_fps_limiter.start_now();

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

		window.clear();
		m_map.print(window);
		ImGui::SFML::Render(window);
		window.display();

		m_fps_limiter.wait();
	}
	if (window.isOpen()) {
		window.close();
	}
	ImGui::SFML::Shutdown();
}
