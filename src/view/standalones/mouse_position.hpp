#ifndef NINJACLOWN_VIEW_STANDALONES_MOUSE_POSITION_HPP
#define NINJACLOWN_VIEW_STANDALONES_MOUSE_POSITION_HPP

namespace sf {
class RenderWindow;

template <typename T>
class Vector2;
typedef Vector2<float> Vector2f;

} // namespace sf

namespace view {


/**
 * Returns the position of the mouse in screen coord, taking the view port into account
 */
sf::Vector2f get_mouse_pos(const sf::RenderWindow&) noexcept;

} // namespace view

#endif //NINJACLOWN_VIEW_STANDALONES_MOUSE_POSITION_HPP
