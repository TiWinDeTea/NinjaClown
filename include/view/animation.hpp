#ifndef NINJACLOWN_VIEW_ANIMATION_HPP
#define NINJACLOWN_VIEW_ANIMATION_HPP

#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace view {
class animation {
public:
    static constexpr unsigned int SINGLE_IMAGE_DURATION = 7;

    void add_frame(sf::Sprite&& sprite) {
        m_frames.emplace_back(std::move(sprite));
    }

    void print(sf::RenderWindow& window, float posx, float posy) const noexcept;

private:
    mutable std::vector<sf::Sprite> m_frames;
};

class shifted_animation {
public:
    static constexpr unsigned int SINGLE_IMAGE_DURATION = animation::SINGLE_IMAGE_DURATION;

    void add_frame(sf::Sprite&& sprite) {
        m_frames.emplace_back(std::move(sprite));
    }

    void set_shift(float xshift, float yshift) {
        m_xshift = xshift;
        m_yshift = yshift;
    }

    void print(sf::RenderWindow& window, float posx, float posy) const noexcept;

private:
    mutable std::vector<sf::Sprite> m_frames;
    float m_xshift;
    float m_yshift;
};
}
#endif //NINJACLOWN_VIEW_ANIMATION_HPP
