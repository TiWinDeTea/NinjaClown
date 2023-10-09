#ifndef NINJACLOWN_VIEW_OBJECTS_HPP
#define NINJACLOWN_VIEW_OBJECTS_HPP

#include "view/overmap_displayable.hpp"

#include <memory>

namespace utils {
class resource_manager;
}

namespace view {
class shifted_animation;
}

namespace view {

class object: public overmap_displayable_interface {
public:
	object() noexcept = default;
	object(object&&) noexcept = default;
    virtual ~object();

	void set_pos(float x, float y) {
		p_posx = x + m_xshift;
		p_posy = y + m_yshift;
	}

	void set_id(utils::resources_type::object_id id);

	void reload_sprites();

private:
	void vprint(map_viewer &view) const override;

	bool vis_hovered(map_viewer &view) const noexcept override;

	utils::resources_type::object_id m_object_id;


    std::unique_ptr<shifted_animation> m_animation;
	float m_xshift{0.f};
	float m_yshift{0.f};
};
} // namespace view

#endif //NINJACLOWN_VIEW_OBJECTS_HPP
