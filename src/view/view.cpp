#include "view/view.hpp"
#include "adapter/adapter.hpp"
#include "state_holder.hpp"
#include "terminal_commands.hpp"
#include "view/game_viewer.hpp"
#include "view/menu.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <imgui-SFML.h>
#include <imterm/terminal.hpp>
#include <memory>
#include <thread>

namespace {
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

#include "model/model.hpp"

void view::view::do_run(state::holder &state) {

	sf::RenderWindow window{sf::VideoMode{1600, 900}, "Ninja clown !"};
	window.setFramerateLimit(std::numeric_limits<unsigned int>::max());
	window.clear();

	ImTerm::terminal<terminal_commands> &terminal = state::access<view>::terminal(state);
	terminal.set_width(1600);
	terminal.set_height(400);
	terminal.theme() = ImTerm::themes::cherry;
	terminal.log_level(ImTerm::message::severity::info);
	terminal.set_flags(ImGuiWindowFlags_NoTitleBar);
	terminal.disallow_x_resize();
	terminal.filter_hint() = "regex filter..."; // TODO traduiction

	menu menu{state};

	game_viewer game{window, state};

	m_game = &game;
	m_menu = &menu;

	ImGui::SFML::Init(window);
	ImGui::GetIO().IniFilename = nullptr;

	m_fps_limiter.start_now();
	sf::Clock clock{};

//	state::access<::view::view>::adapter(state).load_map("resources/map_test.map");
//	state::access<::view::view>::model(state).load_dll("ninja-clown-basic-bot.dll");
//	state::access<::view::view>::model(state).run();

	while (m_running.test_and_set() && window.isOpen()) {
		ImGui::SFML::Update(window, clock.restart());

		sf::Event event{};
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::F11) {
					m_showing_term = !m_showing_term;
				}
				else if (event.key.code == sf::Keyboard::Escape) {
					if (m_showing == window::menu) {
						m_showing = window::game;
						menu.close();
						game.resume();
					}
					else {
						m_showing = window::menu;
						game.pause();
					}
				}
			}

			if (event.type == sf::Event::Closed) {
				window.close();
			}

			const bool is_mouse_event = mouse_type_event(event.type);
			if (!is_mouse_event || (sf::Mouse::getPosition(window).y > terminal.get_size().y + 2 && m_showing_term)) {
				game.event(event);
			}

			if (m_showing_term || m_showing == window::menu) {
				ImGui::SFML::ProcessEvent(event);
			}
		}

		if (m_showing == window::menu) {
			auto request = menu.show();
			switch (request) {
				case menu::user_request::none:
					break;
				case menu::user_request::close_menu:
					m_showing = window::game;
					game.resume();
					break;
				case menu::user_request::close_window:
					m_running.clear();
					break;
				case menu::user_request::restart:
					state::access<view>::adapter(state).load_map(state.current_map_path());
					m_showing = window::game;
					game.restart();
					break;
				case menu::user_request::load_dll:
					terminal_commands::argument_type arg{state, terminal, {}};
					arg.command_line.emplace_back();
					arg.command_line.push_back(menu.path().generic_string());
					terminal_commands::load_shared_library(arg);
					break;
			}
		}

		if (m_showing_term) {
			ImGui::SetNextWindowPos(ImVec2{0, 0}, ImGuiCond_Always);
			terminal.show();
		}

		// somebody help me
        sf::RectangleShape rect;
        rect.setFillColor(sf::Color::Transparent);
        rect.setSize({0,0});
        rect.setPosition(0, 0);
        window.draw(rect);

		game.show(show_debug_data);

        ImGui::SFML::Render();

		window.display();
		m_fps_limiter.wait();
		window.clear();
	}

	m_game = nullptr;
	m_menu = nullptr;
}

bool view::view::has_map() const noexcept {
	return m_game != nullptr && m_game->has_map();
}
