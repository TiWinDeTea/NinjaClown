#include "view/animation.hpp"
#include "program_state.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <spdlog/spdlog.h>

void view::animation::print(sf::RenderWindow &window, float posx, float posy) const noexcept
{
	const auto& tiles = program_state::global->resource_manager.tiles_infos();
	auto &viewer      = program_state::global->viewer;

	auto level_size = viewer.level_size();
	auto xshift     = static_cast<float>(level_size.second * tiles.y_xshift);

	auto selected_frame = (viewer.current_frame() / SINGLE_IMAGE_DURATION) % m_frames.size();
	sf::Sprite &frame   = m_frames[selected_frame];
	frame.setPosition(posx * static_cast<float>(tiles.xspacing) + posy * static_cast<float>(-tiles.y_xshift) + xshift,
	                  posy * static_cast<float>(tiles.yspacing) + posx * static_cast<float>(tiles.x_yshift));
	window.draw(frame);
}


void view::shifted_animation::print(sf::RenderWindow &window, float posx, float posy) const noexcept
{
    const auto &tiles = program_state::global->resource_manager.tiles_infos();
    auto &viewer      = program_state::global->viewer;

    auto level_size = viewer.level_size();
    auto shift      = static_cast<float>(level_size.second * tiles.y_xshift);

    auto selected_frame = (viewer.current_frame() / SINGLE_IMAGE_DURATION) % m_frames.size();
    sf::Sprite &frame   = m_frames[selected_frame];
    frame.setPosition(posx * static_cast<float>(tiles.xspacing) + posy * static_cast<float>(-tiles.y_xshift) + shift + m_xshift,
                      posy * static_cast<float>(tiles.yspacing) + posx * static_cast<float>(tiles.x_yshift) + m_yshift);
    window.draw(frame);
}