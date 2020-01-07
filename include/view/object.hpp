#ifndef NINJACLOWN_VIEW_OBJECTS_HPP
#define NINJACLOWN_VIEW_OBJECTS_HPP

#include "view/animation.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <cassert>

namespace view {
class object {
public:
    void print(sf::RenderWindow &window) const {
        assert(m_animation);
        m_animation->print(window, m_posx, m_posy);
    }

    bool is_hovered(sf::RenderWindow& window) const noexcept {
        assert(m_animation);
        return m_animation->is_hovered(window);
    }

    void set_pos(float x, float y) {
        m_posx = x + m_xshift;
        m_posy = y + m_yshift;
    }

    void set_animation(const shifted_animation& animation)
    {
        m_animation = &animation;
    }

private:
    const shifted_animation *m_animation;
    float m_posx;
    float m_posy;
    float m_xshift;
    float m_yshift;
};
}

#endif //NINJACLOWN_VIEW_OBJECTS_HPP
