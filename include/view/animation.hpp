#ifndef NINJACLOWN_VIEW_ANIMATION_HPP
#define NINJACLOWN_VIEW_ANIMATION_HPP

#include <vector>

#include <SFML/Graphics/Sprite.hpp>

namespace view {
class animation {
public:
    void add_frame(sf::Sprite&& sprite) {
        m_frames.emplace_back(std::move(sprite));
    }

private:
    std::vector<sf::Sprite> m_frames;
};
}
#endif //NINJACLOWN_VIEW_ANIMATION_HPP
