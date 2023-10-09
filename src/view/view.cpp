#include "adapter/adapter.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"
#include "terminal_commands.hpp"
#include "utils/resource_manager.hpp"
#include "view/game_viewer.hpp"
#include "view/menu.hpp"
#include "view/view.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <SFML/Window/Event.hpp>
#include <imgui-SFML.h>
#include <imterm/terminal.hpp>
#include <memory>

#include <thread>
#include <spdlog/spdlog.h>

using fmt::operator""_a;

namespace {
void setup_terminal(ImTerm::terminal<terminal_commands> &terminal, unsigned int x_size, unsigned int y_size) {
	terminal.set_width(x_size);
	terminal.set_height(y_size);
	terminal.theme() = ImTerm::themes::cherry;
	terminal.log_level(ImTerm::message::severity::info);
	terminal.set_flags(ImGuiWindowFlags_NoTitleBar);
	terminal.disallow_x_resize();
	terminal.filter_hint() = "regex filter..."; // TODO traduction
}

bool mouse_type_event(sf::Event::EventType et) {
	switch (et) {
		case sf::Event::MouseWheelMoved:
		case sf::Event::MouseWheelScrolled:
		case sf::Event::MouseButtonPressed:
		case sf::Event::MouseButtonReleased:
		case sf::Event::MouseMoved:
		case sf::Event::MouseEntered:
		case sf::Event::MouseLeft:
		case sf::Event::TouchBegan:
		case sf::Event::TouchMoved:
		case sf::Event::TouchEnded:
			return true;
		default:
			return false;
	}
}
} // namespace

view::view::~view() {
	assert(!m_thread || !m_thread->joinable());
}

void view::view::exec(state::holder &state) {
	assert(!m_thread);

	m_running.test_and_set();
	m_thread = std::make_unique<std::thread>([&]() {
		do_run(state);
	});
}

void view::view::do_run(state::holder &state) {
	const auto& resources = utils::resource_manager::instance();

	constexpr unsigned int x_window_size = 1600;
	constexpr unsigned int y_window_size = 900;

	sf::RenderWindow window{sf::VideoMode{x_window_size, y_window_size}, "Ninja clown !"};
	window.setFramerateLimit(std::numeric_limits<unsigned int>::max());
	window.clear();

	ImTerm::terminal<terminal_commands> &terminal = state::access<view>::terminal(state);
	setup_terminal(terminal, x_window_size, y_window_size / 3);

	menu menu{state};
	game_viewer game{window, state};

	m_game = &game;
	m_menu = &menu;

	ImGui::SFML::Init(window);
	ImGui::GetIO().IniFilename = nullptr;

	m_fps_limiter.start_now();
	sf::Clock clock{};

	state::access<::view::view>::adapter(state).load_map("resources/maps/map_test/map_test.map"); // TODO remove at some point

	while (m_running.test_and_set() && window.isOpen()) {
		ImGui::SFML::Update(window, clock.restart());
		manage_events(window, static_cast<unsigned int>(terminal.get_size().y));
		auto restore_view = window.getView();

		if (m_showing == window::menu) {
			auto request = menu.show();
			switch (request) {
				case menu::user_request::none:
					break;
				case menu::user_request::close_menu:
					m_showing = window::game;
					game.resume();
					menu.close();
					break;
				case menu::user_request::close_window:
					m_running.clear();
					break;
				case menu::user_request::restart:
					state::access<view>::adapter(state).load_map(state.current_map_path());
					m_showing = window::game;
					game.restart();
					menu.close();
					break;
				case menu::user_request::load_dll: {
					terminal_commands::argument_type arg{state, terminal, {}};
					arg.command_line.emplace_back();
					arg.command_line.push_back(menu.path().generic_string());
					terminal_commands::load_shared_library(arg);
					break;
				}
				case menu::user_request::load_map: {
					terminal_commands::argument_type arg{state, terminal, {}};
					arg.command_line.emplace_back();
					arg.command_line.push_back(menu.path().generic_string());
					terminal_commands::load_map(arg);
					break;
				}
				default:
					spdlog::error(resources.log_for("view.view.menu.unknown_request"), "id"_a=static_cast<int>(request));
					break;
			}
		}

		if (m_showing_term) {
			ImGui::SetNextWindowPos(ImVec2{0, 0}, ImGuiCond_Always);
			terminal.show();
		}

		game.show(show_debug_data);
		ImGui::SFML::Render();

		{ // Fixxy doo fix, fixes linux display bug somehow
			sf::RectangleShape rect;
			rect.setFillColor(sf::Color::Transparent);
			rect.setPosition(-10.f, -10.f);
			window.draw(rect);
		}

		window.setView(restore_view);
		window.display();
		m_fps_limiter.wait();
		window.clear();
	}

	m_game = nullptr;
	m_menu = nullptr;
}

void view::view::manage_events(sf::RenderWindow &window, unsigned int terminal_height) noexcept {
	sf::Event event{};
	while (window.pollEvent(event)) {
		if (event.type == sf::Event::KeyPressed) {
			if (event.key.code == sf::Keyboard::F11) {
				m_showing_term = !m_showing_term;
			}
			else if (event.key.code == sf::Keyboard::Escape) {
				if (m_showing == window::menu) {
					m_showing = window::game;
					m_menu->close();
					m_game->resume();
				}
				else {
					m_showing = window::menu;
					m_game->pause();
				}
			}
		}

		if (event.type == sf::Event::Closed) {
			window.close();
		}

		if (m_showing != window::menu
		    && (!mouse_type_event(event.type) || sf::Mouse::getPosition(window).y > terminal_height + 2 || !m_showing_term)) {
			m_game->event(event);
		}

		ImGui::SFML::ProcessEvent(event);
	}
}

bool view::view::has_map() const noexcept {
	return m_game != nullptr && m_game->has_map();
}
