#include "view/animation.hpp"
#include "program_state.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include <spdlog/spdlog.h>

namespace {
void print_tile(sf::RenderWindow &window, sf::Sprite &frame, float x, float y, float xshift = 0.f, float yshift = 0.f) noexcept
{
	auto [screen_x, screen_y] = program_state::global->viewer.to_screen_coords(x, y);
	frame.setPosition(screen_x + xshift, screen_y + yshift);
	window.draw(frame);
}

sf::Sprite &select(std::vector<sf::Sprite> &sprites, unsigned int image_duration)
{
    auto &viewer = program_state::global->viewer;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - viewer.starting_time);
	return sprites[(duration.count() / image_duration) % sprites.size()];
}
} // namespace

void view::animation::print(sf::RenderWindow &window, float posx, float posy) const noexcept
{
	print_tile(window, select(m_frames, SINGLE_IMAGE_DURATION), posx, posy);
}

void view::animation::highlight(sf::RenderWindow &window, float posx, float posy) const noexcept
{
	sf::Sprite frame = select(m_frames, SINGLE_IMAGE_DURATION);
	frame.setColor(sf::Color{128, 255, 128});
	print_tile(window, frame, posx, posy);
}

void view::shifted_animation::print(sf::RenderWindow &window, float posx, float posy) const noexcept
{
	print_tile(window, select(m_frames, SINGLE_IMAGE_DURATION), posx, posy, m_xshift, m_yshift);
}

void view::shifted_animation::adjust_for_mobs() noexcept
{
	const auto &tiles = program_state::global->resource_manager.tiles_infos();
	auto bounds       = m_frames.front().getLocalBounds();

	m_xshift = 0.f;
	m_yshift = 0.f;
}

bool view::shifted_animation::is_hovered(sf::RenderWindow &window) const noexcept
{
	auto &viewer        = program_state::global->viewer;
	auto selected_frame = (viewer.current_frame() / SINGLE_IMAGE_DURATION) % m_frames.size();
	auto mouse_pos      = sf::Mouse::getPosition(window);
	return m_frames[selected_frame].getGlobalBounds().contains(static_cast<float>(mouse_pos.x), static_cast<float>(mouse_pos.y));
}
