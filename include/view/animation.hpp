#ifndef NINJACLOWN_VIEW_ANIMATION_HPP
#define NINJACLOWN_VIEW_ANIMATION_HPP

#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace view {
class animation {
    static constexpr unsigned int SINGLE_IMAGE_DURATION = 7;

public:
    void add_frame(sf::Sprite&& sprite) {
        m_frames.emplace_back(std::move(sprite));
    }

    void print(sf::RenderWindow& window, float posx, float posy) const noexcept;

private:
    mutable std::vector<sf::Sprite> m_frames;
};
}
#endif //NINJACLOWN_VIEW_ANIMATION_HPP
