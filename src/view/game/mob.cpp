#include <cassert>

#include "mob.hpp"
#include "utils/resource_manager.hpp"

view::mob::~mob() = default;

void view::mob::set_mob_id(utils::resources_type::mob_id id) {
	m_mob_id = id;
	reload_sprites();
}

void view::mob::reload_sprites() {
	auto anim = utils::resource_manager::instance().mob_animations(m_mob_id);
	assert(anim);
    m_animations = std::make_unique<view::mob_animations>(*anim);
}

void view::mob::set_direction(facing_direction::type dir) {
	m_dir = dir;
}

void view::mob::vprint(view::map_viewer &view) const {
	const auto &anim = m_animations->animation_for(m_dir);
	assert(anim);
	anim->print(view, p_posx, p_posy);
}

bool view::mob::vis_hovered(view::map_viewer &view) const noexcept {
	const auto &anim = m_animations->animation_for(m_dir);
	assert(anim);
	return anim->is_hovered(view);
}