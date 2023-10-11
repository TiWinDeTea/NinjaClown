#include "object.hpp"
#include "utils/resource_manager.hpp"
#include "utils/resources_type.hpp"
#include "view/assets/animation.hpp"

view::object::~object() = default;

void view::object::set_id(utils::resources_type::object_id id) {
	m_object_id = id;
	reload_sprites();
}

void view::object::reload_sprites() {
	utils::optional<const shifted_animation &> animation = utils::resource_manager::instance().object_animation(m_object_id);
	assert(animation);
	m_animation = std::make_unique<view::shifted_animation>(*animation);
}

void view::object::vprint(view::map_viewer &view) const {
	assert(!m_animation->empty());
	m_animation->print(view, p_posx, p_posy);
}

bool view::object::vis_hovered(view::map_viewer &view) const noexcept {
	assert(!m_animation->empty());
	return m_animation->is_hovered(view);
}
