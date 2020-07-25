#ifndef NINJACLOWN_VIEWER_DISPLAY_STATE_HPP
#define NINJACLOWN_VIEWER_DISPLAY_STATE_HPP

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>

#include <optional>

#include "terminal_commands.hpp"

#include "view/configurator.hpp"
#include "view/file_explorer.hpp"

namespace ImTerm {
template <typename>
class terminal;
}

namespace view {
struct viewer_display_state {

	view::configurator configurator;

	sf::RenderWindow window;
	ImTerm::terminal<terminal_commands> &terminal;
	terminal_commands::argument_type empty_arg;
	sf::Clock delta_clock{};
	sf::FloatRect viewport{};

	sf::Vector2i window_size{};

	// events related ; empty if on terminal
	sf::Vector2i mouse_pos{};
	std::optional<sf::Vector2i> left_click_pos{};
	std::optional<sf::Vector2i> right_click_pos{};

	// commands/terminal related
	bool terminal_hovered{false};
	bool displaying_term{false};
	bool autostep_bot{false};

	// misc
	bool resized_once{false}; // TODO useless ?
	bool showing_escape_menu{true};
	bool escape_menu_currently_open{false};

	file_explorer explorer{};
};
} // namespace view

#endif //NINJACLOWN_VIEWER_DISPLAY_STATE_HPP
