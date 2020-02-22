#ifndef NINJACLOWN_VIEW_OBJECTS_HPP
#define NINJACLOWN_VIEW_OBJECTS_HPP

#include "view/animation.hpp"
#include "view/overmap_displayable.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <cassert>

namespace view {

class object : public overmap_displayable_interface {
public:
	void vprint(view::viewer &view) const override {
		assert(m_animation);
		m_animation->print(view, p_posx, p_posy);
	}

	bool vis_hovered(view::viewer &view) const noexcept override {
		assert(m_animation);
		return m_animation->is_hovered(view);
	}

	void set_pos(float x, float y) {
		p_posx = x + m_xshift;
		p_posy = y + m_yshift;
	}

	void set_animation(const shifted_animation &animation) {
		m_animation = &animation;
	}

private:
	const shifted_animation *m_animation;
	float m_xshift;
	float m_yshift;
};
} // namespace view

#endif //NINJACLOWN_VIEW_OBJECTS_HPP
