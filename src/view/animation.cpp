#include "view/animation.hpp"
#include "view/viewer.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include <spdlog/spdlog.h>

namespace {
void print_tile(view::viewer& viewer, sf::Sprite &frame,
                float x, float y, float xshift = 0.f, float yshift = 0.f) noexcept {
	auto [screen_x, screen_y] = viewer.to_screen_coords(x, y);
	frame.setPosition(screen_x + xshift, screen_y + yshift);
	viewer.window->draw(frame);
}

sf::Sprite &select(std::chrono::system_clock::time_point starting_time, std::vector<sf::Sprite> &sprites, unsigned int image_duration) {
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - starting_time);
	return sprites[(duration.count() / image_duration) % sprites.size()];
}
} // namespace

void view::animation::print(view::viewer& viewer, float posx, float posy) const noexcept {
	print_tile(viewer, select(viewer.starting_time, m_frames, SINGLE_IMAGE_DURATION), posx, posy);
}

void view::animation::highlight(view::viewer& viewer, float posx, float posy) const noexcept {
	sf::Sprite frame = select(viewer.starting_time, m_frames, SINGLE_IMAGE_DURATION);
	frame.setColor(sf::Color{128, 255, 128});
	print_tile(viewer, frame, posx, posy);
}

void view::shifted_animation::print(view::viewer& viewer, float posx, float posy) const noexcept {
	print_tile(viewer, select(viewer.starting_time, m_frames, SINGLE_IMAGE_DURATION), posx, posy, m_xshift, m_yshift);
}

bool view::shifted_animation::is_hovered(view::viewer& viewer) const noexcept {
	auto selected_frame = (viewer.current_frame() / SINGLE_IMAGE_DURATION) % m_frames.size();
	return m_frames[selected_frame].getGlobalBounds().contains(viewer.get_mouse_pos());
}
