#include "view/animation.hpp"
#include "program_state.hpp"

#include <SFML/Graphics/RenderWindow.hpp>

void view::animation::print(sf::RenderWindow &window, int posx, int posy) const noexcept
{
	const auto &tiles = program_state::global->resource_manager.tiles_infos();
	auto& viewer = program_state::global->viewer;

	auto level_size = viewer.level_size();
	auto shift = level_size.second * tiles.y_xshift;

	auto selected_frame = (viewer.current_frame() / SINGLE_IMAGE_DURATION) % m_frames.size();
	sf::Sprite &frame   = m_frames[selected_frame];
	frame.setPosition(static_cast<float>(posx * tiles.xspacing + posy * -tiles.y_xshift + shift),
	                  static_cast<float>(posy * tiles.yspacing + posx * tiles.x_yshift));
	window.draw(frame);
}
