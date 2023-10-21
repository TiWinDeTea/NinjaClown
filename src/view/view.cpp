#include "view/view.hpp"
#include "adapter/adapter.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"
#include "terminal_commands.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"
#include "utils/system.hpp"
#include "view/game/game_viewer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <SFML/Window/Event.hpp>
#include <imgui-SFML.h>
#include <imterm/terminal.hpp>
#include <memory>

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <spdlog/spdlog.h>
#include <thread>

using fmt::operator""_a;

namespace {
constexpr const auto fontawesome_path = "fonts/fontawesome-free-5.4.0-desktop/" FONT_ICON_FILE_NAME_FAS;

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
	constexpr unsigned int x_window_size = 1600;
	constexpr unsigned int y_window_size = 900;

	sf::RenderWindow window{sf::VideoMode{x_window_size, y_window_size}, "Ninja clown !"};
	window.setFramerateLimit(std::numeric_limits<unsigned int>::max());
	window.clear();

	ImTerm::terminal<terminal_commands> &terminal = state::access<view>::terminal(state);
	setup_terminal(terminal, x_window_size, y_window_size / 3);
	game_viewer game{window, state};

	m_game = &game;

	ImGui::SFML::Init(window);
	ImGui::GetIO().IniFilename = nullptr;

	m_fps_limiter.start_now();
	sf::Clock clock{};

	state::access<::view::view>::adapter(state).load_map("resources/maps/map_test/map_test.map"); // TODO remove at some point

	constexpr std::array<ImWchar, 3> fontawesome_icons_ranges = {ICON_MIN_FA, ICON_MAX_FA, 0};
	ImFontConfig fontawesome_icons_config{};
	fontawesome_icons_config.MergeMode        = true;
	fontawesome_icons_config.PixelSnapH       = true;
	fontawesome_icons_config.GlyphMinAdvanceX = 13.5f;

	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->AddFontDefault(&fontawesome_icons_config);

	auto *fontawesome = io.Fonts->AddFontFromFileTTF(
	  (utils::resources_directory() / fontawesome_path).generic_string().c_str(), 13.5f,
	  &fontawesome_icons_config, fontawesome_icons_ranges.data());

	if (fontawesome == nullptr) {
		utils::log::warn("view.view.FA_load_failed");
	}
	else {
		utils::log::debug("view.view.FA_load_success");
		ImGui::SFML::UpdateFontTexture();
	}

	while (m_running.test_and_set() && window.isOpen()) {
		ImGui::SFML::Update(window, clock.restart());
		manage_events(window, state);
		auto restore_view = window.getView();

		switch (m_show_state) {
			case window::game:
				if (!game.show(show_debug_data)) {
					m_show_state = window::menu;
				}
				break;
			case window::menu:
				// TODO : show main menu
				break;
			case window::map_editor:
				// TODO : show map editor
			default:
				utils::log::warn("view.view.bad_state", "state"_a = static_cast<int>(m_show_state));
		}

		if (m_showing_term) {
			ImGui::SetNextWindowPos(ImVec2{0, 0}, ImGuiCond_Always);
			terminal.show();
		}

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
}

void view::view::manage_events(sf::RenderWindow &window, state::holder &state) noexcept {
	sf::Event event{};
	while (window.pollEvent(event)) {
		if (event.type == sf::Event::KeyPressed) {
			if (event.key.code == sf::Keyboard::F12) {
				m_showing_term = !m_showing_term;
			}
		}

		if (event.type == sf::Event::Closed) {
			window.close();
		}

		if (event.type == sf::Event::Resized) {
			state::access<view>::terminal(state).set_width(window.getSize().x);
		}

		switch (m_show_state) {
			case window::game:
				m_game->event(event);
				break;
			case window::menu:
				// TODO : pass events to main menu once it’s developped
				break;
			case window::map_editor:
				// TODO : pass events to map editor once it’s developped
				break;
		}

		ImGui::SFML::ProcessEvent(event);
	}
}

bool view::view::has_map() const noexcept {
	return m_game != nullptr && m_game->has_map();
}
