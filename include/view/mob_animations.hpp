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

    [[nodiscard]] std::optional<animation>& animation_for(facing_direction::type dir) noexcept {
        assert(dir != facing_direction::MAX_VAL);
        return m_animations_by_direction[dir];
    }


    void add_animation(animation&& anim, facing_direction::type dir) noexcept {
        assert(dir != facing_direction::MAX_VAL);
        assert(!m_animations_by_direction[dir]);
        m_animations_by_direction[dir] = std::move(anim);
    }

private:
    std::array<std::optional<animation>, facing_direction::MAX_VAL> m_animations_by_direction{};
};
}
#endif //NINJACLOWN_VIEW_SPRITES_HPP
