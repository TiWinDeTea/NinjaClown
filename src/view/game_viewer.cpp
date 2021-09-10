#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "state_holder.hpp"
#include "view/game_viewer.hpp"

namespace {
const sf::Event::KeyEvent &key(const sf::Event &event) {
	assert(event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased); // NOLINT
	return event.key; // NOLINT
}

const sf::Event::SizeEvent &size(const sf::Event &event) {
	assert(event.type == sf::Event::Resized); // NOLINT
	return event.size; // NOLINT
}

const sf::Event::MouseWheelScrollEvent &mouse_wheel_scroll(const sf::Event &event) {
	assert(event.type == sf::Event::MouseWheelScrolled); // NOLINT
	return event.mouseWheelScroll; // NOLINT
}

const sf::Event::MouseMoveEvent &mouse_move(const sf::Event &event) {
	assert(event.type == sf::Event::MouseMoved); // NOLINT
	return event.mouseMove; // NOLINT
}

const sf::Event::MouseButtonEvent &mouse_button(const sf::Event &event) {
	assert(event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased); // NOLINT
	return event.mouseButton; // NOLINT
}
} // namespace

view::game_viewer::game_viewer(sf::RenderWindow &window, state::holder &state) noexcept
    : m_window{window}
    , m_state{state}
    , m_map{state}
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


void view::game_viewer::show(bool show_debug_data) {
    m_window_size = m_window.getSize();
	m_map.print(show_debug_data);
}

void view::game_viewer::event(const sf::Event &event) {
	switch (event.type) {
		case sf::Event::KeyPressed:
			switch (key(event).code) {
				case sf::Keyboard::F5:
					if (!m_autostep_bot) {
						terminal_commands::update_world(m_fake_arg);
					}
					break;
				case sf::Keyboard::F4:
					if (std::exchange(m_autostep_bot, !m_autostep_bot)) {
						terminal_commands::stop_model(m_fake_arg);
					}
					else {
						terminal_commands::run_model(m_fake_arg);
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

			auto &terminal = state::access<game_viewer>::terminal(m_state);
			terminal.set_width(m_window.getSize().x);
			if (m_resized_once) {
				terminal.set_height(std::min(m_window.getSize().y, static_cast<unsigned>(terminal.get_size().y)));
			}
            m_resized_once = true;

            sf::Event::SizeEvent sz = size(event);
            const float x_ratio     = static_cast<float>(sz.width) / m_window_size.x;
            const float y_ratio     = static_cast<float>(sz.height) / m_window_size.y;

            sf::View view = m_window.getView();
            sf::Vector2f top_left = view.getCenter() - view.getSize() / 2.f;
			view.setSize(view.getSize().x * x_ratio, view.getSize().y * y_ratio);
			view.setCenter(top_left + view.getSize() / 2.f);

            m_window_size.x = sz.width;
            m_window_size.y = sz.height;
            m_window.setView(view);

			break;
		}
		case sf::Event::MouseWheelScrolled: {
			sf::Event::MouseWheelScrollEvent wheel_scroll = mouse_wheel_scroll(event);
			if (wheel_scroll.wheel == sf::Mouse::Wheel::VerticalWheel) {
                sf::View view     = m_window.getView();

				const float delta = 1.1f;
				float transform;
				if (wheel_scroll.delta < 0) {
					transform = delta;
				}
				else {
					transform = 1 / delta;
				}

				const float vp2win_ratio_x = view.getSize().x / m_window_size.x;
				const float vp2win_ratio_y = view.getSize().y / m_window_size.y;
				const auto wheel_x         = static_cast<float>(wheel_scroll.x);
				const auto wheel_y         = static_cast<float>(wheel_scroll.y);

				sf::Vector2f top_left = view.getCenter() - view.getSize() / 2.f + sf::Vector2f{wheel_x * (1 - transform) * vp2win_ratio_x, wheel_y * (1 - transform) * vp2win_ratio_y};
                view.zoom(transform);
                view.setCenter(top_left + view.getSize() / 2.f);

				m_window.setView(view);
			}
		} break;
		case sf::Event::MouseMoved:
			{
				sf::Event::MouseMoveEvent move = mouse_move(event);
				if (m_left_click_pos) {
				    sf::View view = m_window.getView();
					const float zoom_factor = m_window.getSize().x / view.getSize().x;

				    view.move(static_cast<float>(m_mouse_pos.x - move.x) / zoom_factor, static_cast<float>(m_mouse_pos.y - move.y) / zoom_factor);
                    m_window.setView(view);
				}
                m_mouse_pos        = {move.x, move.y};
				break;
			}
		case sf::Event::MouseButtonReleased:
			switch (sf::Event::MouseButtonEvent button = mouse_button(event); button.button) {
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
			switch (sf::Event::MouseButtonEvent button = mouse_button(event); button.button) {
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
