#ifndef NINJACLOWN_VIEW_OBJECTS_HPP
#define NINJACLOWN_VIEW_OBJECTS_HPP

#include "animation.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <cassert>

namespace view {
class object {
public:
    void print(sf::RenderWindow &window) const {
        assert(m_animation);
        m_animation->print(window, m_posx, m_posy);
    }

    void set_pos(float x, float y) {
        m_posx = x;
        m_posy = y;
    }

    void set_animation(const animation& animation)
    {
        m_animation = &animation;
    }

private:
    const animation *m_animation;
    float m_posx;
    float m_posy;
};
}

#endif //NINJACLOWN_VIEW_OBJECTS_HPP
