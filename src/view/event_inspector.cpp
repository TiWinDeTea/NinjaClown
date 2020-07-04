#include <SFML/Graphics/Rect.hpp>
#include <SFML/Window/Event.hpp>

#include "view/event_inspector.hpp"
#include "view/viewer.hpp"
#include "view/viewer_display_state.hpp"

namespace {
const sf::Event::KeyEvent &key(const sf::Event &event) {
	assert(event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased); // NOLINT
	return event.key;
}

const sf::Event::SizeEvent &size(const sf::Event &event) {
	assert(event.type == sf::Event::Resized); // NOLINT
	return event.size;
}

const sf::Event::MouseWheelScrollEvent &mouse_wheel_scroll(const sf::Event &event) {
	assert(event.type == sf::Event::MouseWheelScrolled); // NOLINT
	return event.mouseWheelScroll;
}

const sf::Event::MouseMoveEvent &mouse_move(const sf::Event &event) {
	assert(event.type == sf::Event::MouseMoved); // NOLINT
	return event.mouseMove;
}

const sf::Event::MouseButtonEvent &mouse_button(const sf::Event &event) {
	assert(event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased); // NOLINT
	return event.mouseButton;
}
} // namespace

void view::inspect_event(viewer &viewer, const sf::Event &event, viewer_display_state &state) {
#define NEEDS_NOPOPUP                                                                                                                      \
	do {                                                                                                                                   \
		if (state.showing_escape_menu) {                                                                                                   \
			return;                                                                                                                        \
		}                                                                                                                                  \
	} while (false)

	switch (event.type) {
		case sf::Event::Closed:
			state.window.close();
			break;
		case sf::Event::KeyPressed:
			switch (key(event).code) {
				case sf::Keyboard::F11:
					NEEDS_NOPOPUP;
					if (state.displaying_term) {
						state.displaying_term  = false;
						state.terminal_hovered = false;
					}
					else {
						state.displaying_term = true;
					}
					break;
				case sf::Keyboard::F5:
					NEEDS_NOPOPUP;
					if (!state.autostep_bot) {
						terminal_commands::update_world(state.empty_arg);
					}
					break;
				case sf::Keyboard::F4:
					NEEDS_NOPOPUP;
					if (std::exchange(state.autostep_bot, !state.autostep_bot)) {
						terminal_commands::stop_model(state.empty_arg);
					}
					else {
						terminal_commands::run_model(state.empty_arg);
					}
					break;
				case sf::Keyboard::Escape:
					if (!state.displaying_term) {
						if (!state.showing_escape_menu) {
							ImGui::OpenPopup("##in game menu popup");
							state.showing_escape_menu = true;
						}
						else {
							state.showing_escape_menu = false;
						}
					} else {
						state.displaying_term = false;
					}
					break;
				default:
					break;
			}
			break;
		case sf::Event::Resized: {
			state.terminal.set_width(state.window.getSize().x);
			if (state.resized_once) {
				state.terminal.set_height(std::min(state.window.getSize().y, static_cast<unsigned>(state.terminal.get_size().y)));
			}
			state.resized_once = true;

			{
				sf::Event::SizeEvent sz = size(event);
				const float XRATIO      = static_cast<float>(sz.width) / state.window_size.x;
				const float YRATIO      = static_cast<float>(sz.height) / state.window_size.y;

				state.viewport.width *= XRATIO;
				state.viewport.height *= YRATIO;

				state.window_size.x = sz.width;
				state.window_size.y = sz.height;
				state.window.setView(sf::View{state.viewport});
			}
			break;
		}
		case sf::Event::MouseWheelScrolled:
			NEEDS_NOPOPUP;
			if (state.terminal_hovered) {
				return;
			}
			{
				sf::Event::MouseWheelScrollEvent wheel_scroll = mouse_wheel_scroll(event);
				if (wheel_scroll.wheel == sf::Mouse::Wheel::VerticalWheel) {
					const auto MOUSE_POS = viewer.to_viewport_coord(sf::Vector2i{wheel_scroll.x, wheel_scroll.y});

					const float DELTA = 1.1F;
					float transform;
					if (wheel_scroll.delta < 0) {
						transform = DELTA;
					}
					else {
						transform = 1 / DELTA;
					}

					sf::FloatRect new_viewport;

					new_viewport.width      = state.viewport.width * transform;
					const float WIDTH_RATIO = new_viewport.width / state.viewport.width;
					new_viewport.left       = state.viewport.left + MOUSE_POS.x * (1 - WIDTH_RATIO);

					new_viewport.height      = state.viewport.height * WIDTH_RATIO;
					const float HEIGHT_RATIO = new_viewport.height / state.viewport.height;
					new_viewport.top         = state.viewport.top + MOUSE_POS.y * (1 - HEIGHT_RATIO);

					state.viewport = new_viewport;
					state.window.setView(sf::View{state.viewport});
				}
			}
			break;
		case sf::Event::MouseMoved:
			NEEDS_NOPOPUP;
			{
				sf::Event::MouseMoveEvent move = mouse_move(event);
				if (state.left_click_pos) {
					const float zoom_factor = state.window.getSize().x / state.viewport.width;

					state.viewport.left += static_cast<float>(state.mouse_pos.x - move.x) / zoom_factor;
					state.viewport.top += static_cast<float>(state.mouse_pos.y - move.y) / zoom_factor;
					state.window.setView(sf::View{state.viewport});
				}
				state.terminal_hovered = state.displaying_term && state.terminal.get_size().y > static_cast<float>(move.y);
				state.mouse_pos        = {move.x, move.y};
				break;
			}
		case sf::Event::MouseButtonReleased:
			NEEDS_NOPOPUP;
			switch (sf::Event::MouseButtonEvent button = mouse_button(event); button.button) {
				case sf::Mouse::Button::Left:
					if (!state.terminal_hovered) {
						viewer.m_dialog_viewer.on_click(button.x, button.y);
					}
					state.left_click_pos.reset();
					break;
				case sf::Mouse::Button::Right:
					state.right_click_pos.reset();
					break;
				default:
					break;
			}
			break;
		case sf::Event::MouseButtonPressed:
			NEEDS_NOPOPUP;
			switch (sf::Event::MouseButtonEvent button = mouse_button(event); button.button) {
				case sf::Mouse::Button::Left:
					if (!state.terminal_hovered) {
						state.left_click_pos = {button.x, button.y};
					}
					break;
				case sf::Mouse::Button::Right:
					if (!state.terminal_hovered) {
						state.right_click_pos = {button.x, button.y};
					}
					break;
				default:
					break;
			}
			break;
		case sf::Event::LostFocus:
			break;
		case sf::Event::GainedFocus:
			break;
		case sf::Event::TextEntered:
			break;
		case sf::Event::KeyReleased:
			break;
		case sf::Event::MouseWheelMoved:
			break;
		case sf::Event::MouseEntered:
			break;
		case sf::Event::MouseLeft:
			break;
		case sf::Event::JoystickButtonPressed:
			break;
		case sf::Event::JoystickButtonReleased:
			break;
		case sf::Event::JoystickMoved:
			break;
		case sf::Event::JoystickConnected:
			break;
		case sf::Event::JoystickDisconnected:
			break;
		case sf::Event::TouchBegan:
			break;
		case sf::Event::TouchMoved:
			break;
		case sf::Event::TouchEnded:
			break;
		case sf::Event::SensorChanged:
			break;
		case sf::Event::Count:
			break;
		default:
			break;
	}
}
