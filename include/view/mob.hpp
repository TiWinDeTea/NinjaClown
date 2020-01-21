#ifndef NINJACLOWN_VIEW_MOB_HPP
#define NINJACLOWN_VIEW_MOB_HPP

#include <SFML/Graphics/RenderWindow.hpp>

#include "view/mob_animations.hpp"

namespace view {
class mob {
public:
	void print(sf::RenderWindow &window) const {
		const auto &anim = m_animations->animation_for(m_dir);
		assert(anim);
		anim->print(window, m_posx, m_posy);
	}

	bool is_hovered(sf::RenderWindow &window) const noexcept {
		const auto &anim = m_animations->animation_for(m_dir);
		assert(anim);
		return anim->is_hovered(window);
	}

	void set_pos(float x, float y) {
		m_posx = x;
		m_posy = y;
	}

	void set_animations(const mob_animations &animations) {
		m_animations = &animations;
	}

	void set_direction(facing_direction::type dir) {
		m_dir = dir;
	}

private:
	const mob_animations *m_animations;
	float m_posx;
	float m_posy;
	facing_direction::type m_dir;
};
} // namespace view

#endif //NINJACLOWN_VIEW_MOB_HPP
