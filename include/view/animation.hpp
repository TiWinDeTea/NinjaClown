#ifndef NINJACLOWN_VIEW_ANIMATION_HPP
#define NINJACLOWN_VIEW_ANIMATION_HPP

#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace view {
class animation {
public:
    static constexpr unsigned int SINGLE_IMAGE_DURATION = 500;

    void add_frame(sf::Sprite&& sprite) {
        m_frames.emplace_back(std::move(sprite));
    }

    void print(sf::RenderWindow& window, float posx, float posy) const noexcept;

    void highlight(sf::RenderWindow& window, float posx, float posy) const noexcept;

    friend class shifted_animation;

private:
    mutable std::vector<sf::Sprite> m_frames;
};

class shifted_animation {
public:
    static constexpr unsigned int SINGLE_IMAGE_DURATION = animation::SINGLE_IMAGE_DURATION;

    shifted_animation() noexcept(noexcept(std::vector<sf::Sprite>{})) = default;
    shifted_animation(shifted_animation&&) noexcept = default;
    shifted_animation(animation&& o) noexcept : m_frames{std::move(o.m_frames)}{}

    shifted_animation& operator=(shifted_animation&&) noexcept = default;
    shifted_animation& operator=(const shifted_animation&) noexcept = default;

    bool is_hovered(sf::RenderWindow& window) const noexcept;

    void add_frame(sf::Sprite&& sprite) {
        m_frames.emplace_back(std::move(sprite));
    }

    void set_shift(float xshift, float yshift) {
        m_xshift = xshift;
        m_yshift = yshift;
    }

    void adjust_for_mobs() noexcept;

    void print(sf::RenderWindow& window, float posx, float posy) const noexcept;

private:
    mutable std::vector<sf::Sprite> m_frames;
    float m_xshift{};
    float m_yshift{};
};
}
#endif //NINJACLOWN_VIEW_ANIMATION_HPP
