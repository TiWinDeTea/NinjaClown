#ifndef NINJACLOWN_VIEW_MOB_HPP
#define NINJACLOWN_VIEW_MOB_HPP

#include "adapter/facing_dir.hpp"
#include "utils/resources_type.hpp"
#include "view/game/overmap_displayable.hpp"

#include <memory>

namespace utils {
class resource_manager;
}

namespace view {
class mob_animations;
}

namespace view {
class mob: public overmap_displayable_interface {
public:
	mob() noexcept       = default;
	mob(mob &&) noexcept = default;
	virtual ~mob();

	void set_pos(float x, float y) {
		p_posx = x;
		p_posy = y;
	}

	void set_mob_id(utils::resources_type::mob_id id);

	void reload_sprites();

	void set_direction(facing_direction::type dir);

private:
	void vprint(map_viewer &view) const override;

	bool vis_hovered(map_viewer &view) const noexcept override;

	std::unique_ptr<mob_animations> m_animations;
	facing_direction::type m_dir;
	utils::resources_type::mob_id m_mob_id;
};
} // namespace view

#endif //NINJACLOWN_VIEW_MOB_HPP
