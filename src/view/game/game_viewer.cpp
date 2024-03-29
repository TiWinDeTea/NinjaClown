#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include "game_viewer.hpp"
#include "state_holder.hpp"
#include "utils/logging.hpp"
#include "view/standalones/imgui_styles.hpp"

namespace {
const sf::Event::KeyEvent &key(const sf::Event &event) {
	assert(event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased);
	return event.key;
}

const sf::Event::SizeEvent &size(const sf::Event &event) {
	assert(event.type == sf::Event::Resized);
	return event.size;
}

const sf::Event::MouseWheelScrollEvent &mouse_wheel_scroll(const sf::Event &event) {
	assert(event.type == sf::Event::MouseWheelScrolled);
	return event.mouseWheelScroll;
}

const sf::Event::MouseMoveEvent &mouse_move(const sf::Event &event) {
	assert(event.type == sf::Event::MouseMoved);
	return event.mouseMove;
}

const sf::Event::MouseButtonEvent &mouse_button(const sf::Event &event) {
	assert(event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased);
	return event.mouseButton;
}
} // namespace

view::game_viewer::game_viewer(sf::RenderWindow &window, state::holder &state) noexcept
    : m_window{window}
    , m_state{state}
    , m_map{state}
    , m_menu{state}
    , m_fake_arg{state, state::access<game_viewer>::terminal(state), {}} {
	m_map.set_render_window(window);
}

void view::game_viewer::pause() noexcept {
	if (m_autostep_bot) {
		terminal_commands::stop_model(m_fake_arg);
	}
}

void view::game_viewer::resume() noexcept {
	if (m_autostep_bot) {
		terminal_commands::run_model(m_fake_arg);
	}
}

void view::game_viewer::restart() noexcept {
	m_autostep_bot = false;
}

bool view::game_viewer::show(bool show_debug_data) {
	m_window_size = m_window.getSize();
	m_map.print(show_debug_data);
	show_rightmost_bar();
	if (m_showing_menu) {
		display_menu();
	}

	return std::exchange(m_stay_in_game, true);
}

// todo split
void view::game_viewer::event(const sf::Event &event) {
	switch (event.type) {

		case sf::Event::KeyPressed:
			switch (key(event).code) {
				case sf::Keyboard::F5:
					if (!m_autostep_bot) {
						terminal_commands::update_world(m_fake_arg);
					}
					break;
				case sf::Keyboard::F3:
					if (std::exchange(m_autostep_bot, !m_autostep_bot)) {
						terminal_commands::stop_model(m_fake_arg);
					}
					else {
						terminal_commands::run_model(m_fake_arg);
					}
					break;
				case sf::Keyboard::Escape:
					if (std::exchange(m_showing_menu, !m_showing_menu)) {
						m_menu.close();
					}
					break;
				default:
					break;
			}
			break;

		case sf::Event::Resized: {
			if (m_window_size.x == 0 || m_window_size.y == 0) {
				m_window_size.x = size(event).width;
				m_window_size.y = size(event).height;
				break;
			}

			const sf::Event::SizeEvent sz = size(event);
			const float x_ratio           = static_cast<float>(sz.width) / static_cast<float>(m_window_size.x);
			const float y_ratio           = static_cast<float>(sz.height) / static_cast<float>(m_window_size.y);

			sf::View view               = m_window.getView();
			const sf::Vector2f top_left = view.getCenter() - view.getSize() / 2.f;
			view.setSize(view.getSize().x * x_ratio, view.getSize().y * y_ratio);
			view.setCenter(top_left + view.getSize() / 2.f);

			m_window_size.x = sz.width;
			m_window_size.y = sz.height;
			m_window.setView(view);

			break;
		}

		case sf::Event::MouseWheelScrolled: {
			const sf::Event::MouseWheelScrollEvent wheel_scroll = mouse_wheel_scroll(event);
			if (wheel_scroll.wheel == sf::Mouse::Wheel::VerticalWheel) {
				sf::View view = m_window.getView();

				const float delta = 1.1f;
				float transform{};
				if (wheel_scroll.delta < 0) {
					transform = delta;
				}
				else {
					transform = 1 / delta;
				}

				const float vp2win_ratio_x = view.getSize().x / static_cast<float>(m_window_size.x);
				const float vp2win_ratio_y = view.getSize().y / static_cast<float>(m_window_size.y);
				const auto wheel_x         = static_cast<float>(wheel_scroll.x);
				const auto wheel_y         = static_cast<float>(wheel_scroll.y);

				const sf::Vector2f top_left
				  = view.getCenter() - view.getSize() / 2.f
				    + sf::Vector2f{wheel_x * (1 - transform) * vp2win_ratio_x, wheel_y * (1 - transform) * vp2win_ratio_y};
				view.zoom(transform);
				view.setCenter(top_left + view.getSize() / 2.f);

				m_window.setView(view);
			}
			break;
		}

		case sf::Event::MouseMoved: {
			const sf::Event::MouseMoveEvent move = mouse_move(event);
			if (m_left_click_pos) {
				sf::View view           = m_window.getView();
				const float zoom_factor = static_cast<float>(m_window.getSize().x) / view.getSize().x;

				view.move(static_cast<float>(m_mouse_pos.x - move.x) / zoom_factor,
				          static_cast<float>(m_mouse_pos.y - move.y) / zoom_factor);
				m_window.setView(view);
			}
			m_mouse_pos = {move.x, move.y};
			break;
		}

		case sf::Event::MouseButtonReleased:
			switch (const sf::Event::MouseButtonEvent button = mouse_button(event); button.button) {
				case sf::Mouse::Button::Left:
					m_left_click_pos.reset();
					break;
				case sf::Mouse::Button::Right:
					m_right_click_pos.reset();
					break;
				default:
					break;
			}
			break;

		case sf::Event::MouseButtonPressed:
			switch (const sf::Event::MouseButtonEvent button = mouse_button(event); button.button) {
				case sf::Mouse::Button::Left:
					m_left_click_pos = {button.x, button.y};
					break;
				case sf::Mouse::Button::Right:
					m_right_click_pos = {button.x, button.y};
					break;
				default:
					break;
			}
			break;

		default:
			break;
	}
}

void view::game_viewer::display_menu() noexcept {
	using fmt::operator""_a;

	ImTerm::terminal<terminal_commands> &terminal = state::access<game_viewer>::terminal(m_state);

	auto request = m_menu.show();
	switch (request) {
		case game_menu::user_request::none:
			break;
		case game_menu::user_request::close_menu:
			m_showing_menu = false;
			resume();
			m_menu.close();
			break;
		case game_menu::user_request::close_window:
			m_window.close();
			break;
		case game_menu::user_request::restart:
			state::access<game_viewer>::adapter(m_state).load_map(m_state.current_map_path());
			m_showing_menu = false;
			restart();
			m_menu.close();
			break;
		case game_menu::user_request::load_dll: {
			terminal_commands::argument_type arg{m_state, terminal, {}};
			arg.command_line.emplace_back();
			arg.command_line.push_back(m_menu.path().generic_string());
			terminal_commands::load_shared_library(arg);
			break;
		}
		case game_menu::user_request::load_map: {
			terminal_commands::argument_type arg{m_state, terminal, {}};
			arg.command_line.emplace_back();
			arg.command_line.push_back(m_menu.path().generic_string());
			terminal_commands::load_map(arg);
			break;
		}
		case game_menu::user_request::back_to_main_menu:
			m_stay_in_game = false;
			m_menu.close();
			m_showing_menu = false;
			break;
		default:
			utils::log::error("view.view.menu.unknown_request", "id"_a = static_cast<int>(request));
			break;
	}
}

void view::game_viewer::show_rightmost_bar() noexcept {
	using fmt::operator""_a;

	ImVec2 max_text_size{0.f, 0.f};
	auto update_sz = [&max_text_size](std::string_view str) {
		auto size       = ImGui::CalcTextSize(str.data(), str.data() + str.size());
		max_text_size.x = std::max(max_text_size.x, size.x);
		max_text_size.y = std::max(max_text_size.y, size.y);
	};

	const std::string_view play  = ICON_FA_PLAY;
	const std::string_view pause = ICON_FA_PAUSE;
	const std::string_view step  = ICON_FA_PLAY_CIRCLE;
	update_sz(play);
	update_sz(pause);
	update_sz(step);

	const auto &style       = ImGui::GetStyle();
	const auto total_width  = max_text_size.x + style.ItemInnerSpacing.x * 2 + style.ItemSpacing.x * 2;
	const auto total_height = max_text_size.y * 3 + style.ItemInnerSpacing.y * 2 + style.ItemSpacing.y * 2;

	ImGui::SetNextWindowSize(ImVec2{total_width + style.WindowPadding.x * 2, 0.f}, ImGuiCond_Always);
	ImGui::SetNextWindowPos(
	  ImVec2{static_cast<float>(m_window_size.x) - total_width, (static_cast<float>(m_window_size.y) - total_height) / 2.f},
	  ImGuiCond_Always);

	if (ImGui::Begin("view.game.game_viewer.show_rightmost_bar", nullptr,
	                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove // NOLINT(*-signed-bitwise)
	                   | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground)) {

		if (m_autostep_bot) {
			using_style(disabled_button) {
				ImGui::Button(play.data());
			};
			if (ImGui::Button(pause.data())) {
				terminal_commands::stop_model(m_fake_arg);
				m_autostep_bot = false;
			}
			using_style(disabled_button) {
				ImGui::Button(step.data());
			};
		}
		else {
			if (ImGui::Button(play.data())) {
				terminal_commands::run_model(m_fake_arg);
				m_autostep_bot = true;
			}
			using_style(disabled_button) {
				ImGui::Button(pause.data());
			};
			if (ImGui::Button(step.data())) {
				terminal_commands::update_world(m_fake_arg);
			}
		}
	}
	ImGui::End();
}
