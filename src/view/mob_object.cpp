#include <cassert>

#include "utils/resource_manager.hpp"
#include "view/animation.hpp"
#include "view/mob.hpp"
#include "view/object.hpp"



view::mob::~mob() = default;
view::object::~object() = default;

void view::mob::set_mob_id(utils::resources_type::mob_id id, const utils::resource_manager &resources) {
	m_mob_id = id;
	reload_sprites(resources);
}

void view::mob::reload_sprites(const utils::resource_manager &resources) {
	auto anim = resources.mob_animations(m_mob_id);
	assert(anim);
    m_animations = std::make_unique<view::mob_animations>(*anim);
}

void view::mob::set_direction(facing_direction::type dir) {
	m_dir = dir;
}

void view::mob::vprint(view::viewer &view) const {
	const auto &anim = m_animations->animation_for(m_dir);
	assert(anim);
	anim->print(view, p_posx, p_posy);
}

bool view::mob::vis_hovered(view::viewer &view) const noexcept {
	const auto &anim = m_animations->animation_for(m_dir);
	assert(anim);
	return anim->is_hovered(view);
}

void view::object::set_id(utils::resources_type::object_id id, const utils::resource_manager &res) {
	m_object_id = id;
	reload_sprites(res);
}

void view::object::reload_sprites(const utils::resource_manager &res) {
	utils::optional<const shifted_animation &> animation = res.object_animation(m_object_id);
	assert(animation); // NOLINT
	m_animation = std::make_unique<view::shifted_animation>(*animation);
}

void view::object::vprint(view::viewer &view) const {
	assert(!m_animation->empty());
	m_animation->print(view, p_posx, p_posy);
}

bool view::object::vis_hovered(view::viewer &view) const noexcept {
	assert(!m_animation->empty());
	return m_animation->is_hovered(view);
}
