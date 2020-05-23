#ifndef NINJACLOWN_VIEW_MOB_ANIMATIONS_HPP
#define NINJACLOWN_VIEW_MOB_ANIMATIONS_HPP

#include <array>
#include <cassert>
#include <functional>
#include <vector>

#include "view/animation.hpp"
#include "view/facing_dir.hpp"

namespace view {
class mob_animations {
public:
	[[nodiscard]] const std::optional<shifted_animation> &animation_for(facing_direction::type dir) const noexcept {
		assert(dir != facing_direction::MAX_VAL); // NOLINT
		return m_animations_by_direction[dir];
	}

	void add_animation(shifted_animation &&anim, facing_direction::type dir) noexcept {
		assert(dir != facing_direction::MAX_VAL); // NOLINT
		assert(!m_animations_by_direction[dir]); // NOLINT
		m_animations_by_direction[dir] = std::move(anim);
	}

private:
	std::array<std::optional<shifted_animation>, facing_direction::MAX_VAL> m_animations_by_direction{};
};
} // namespace view
#endif //NINJACLOWN_VIEW_SPRITES_HPP
