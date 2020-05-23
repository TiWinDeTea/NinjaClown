#ifndef NINJACLOWN_VIEW_OBJECTS_HPP
#define NINJACLOWN_VIEW_OBJECTS_HPP

#include <SFML/Graphics/RenderWindow.hpp>

#include "view/animation.hpp"
#include "view/overmap_displayable.hpp"
#include "utils/resource_manager.hpp"


namespace view {

class object : public overmap_displayable_interface {
public:
	void set_pos(float x, float y) {
		p_posx = x + m_xshift;
		p_posy = y + m_yshift;
	}

	void set_id(utils::resource_manager::object_id id, const utils::resource_manager& res) {
		m_object_id = id;
		reload_sprites(res);
	}

	void reload_sprites(const utils::resource_manager& res) {
        m_animation = &*res.object_animation(m_object_id);
	}

private:
    void vprint(view::viewer &view) const override {
        assert(m_animation);
        m_animation->print(view, p_posx, p_posy);
    }

    bool vis_hovered(view::viewer &view) const noexcept override {
        assert(m_animation);
        return m_animation->is_hovered(view);
		return false;
    }


    utils::resource_manager::object_id m_object_id;
	const shifted_animation *m_animation;
	float m_xshift;
	float m_yshift;
};
} // namespace view

#endif //NINJACLOWN_VIEW_OBJECTS_HPP
