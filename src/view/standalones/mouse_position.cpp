#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>

#include "mouse_position.hpp"

sf::Vector2f view::get_mouse_pos(const sf::RenderWindow& window) noexcept {
	auto view = window.getView();
	auto size = window.getSize();
	const sf::Vector2f ratio{view.getSize().x / static_cast<float>(size.x), view.getSize().y / static_cast<float>(size.y)};

	sf::Vector2f mouse(sf::Mouse::getPosition(window));
	mouse.x *= ratio.x;
	mouse.y *= ratio.y;

	return mouse + view.getCenter() - view.getSize() / 2.f;
}
