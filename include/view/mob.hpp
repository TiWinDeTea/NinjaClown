#ifndef NINJACLOWN_VIEW_MOB_HPP
#define NINJACLOWN_VIEW_MOB_HPP

#include <SFML/Graphics/RenderWindow.hpp>

#include "view/mob_animations.hpp"
#include "view/overmap_displayable.hpp"
#include "utils/resource_manager.hpp"


namespace view {
class mob : public overmap_displayable_interface {
public:
	void vprint(view::viewer &view) const override {
		const auto &anim = m_animations->animation_for(m_dir);
		assert(anim);
		anim->print(view, p_posx, p_posy);
	}

	bool vis_hovered(view::viewer &view) const noexcept override {
		const auto &anim = m_animations->animation_for(m_dir);
		assert(anim);
		return anim->is_hovered(view);
	}

	void set_pos(float x, float y) {
		p_posx = x;
		p_posy = y;
	}

	void set_mob_id(utils::resource_manager::mob_id id, const utils::resource_manager& resources) {
		m_mob_id = id;
		reload_sprites(resources);
	}

	void reload_sprites(const utils::resource_manager& resources) {
        m_animations = &*resources.mob_animations(m_mob_id);
	}

	void set_direction(facing_direction::type dir) {
		m_dir = dir;
	}

private:
	const mob_animations *m_animations;
	facing_direction::type m_dir;
    utils::resource_manager::mob_id m_mob_id;
};
} // namespace view

#endif //NINJACLOWN_VIEW_MOB_HPP
