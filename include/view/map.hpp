#ifndef NINJACLOWN_VIEW_MAP_HPP
#define NINJACLOWN_VIEW_MAP_HPP

#include <SFML/Graphics/RenderWindow.hpp>

#include <vector>

namespace view {
class map {
public:
    enum class cell {
        iron_tile,
        concrete_tile,
        abyss,
    };

    void set(std::vector<std::vector<cell>>&& cells) {
        m_cells = std::move(cells);
    }

    [[nodiscard]] std::pair<std::size_t, std::size_t> level_size() const noexcept {
        if (m_cells.empty()) {
            return {0u,0u};
        }
        return {m_cells.size(), m_cells.front().size()};
    }

    void print(sf::RenderWindow& window) const noexcept;

private:
    std::vector<std::vector<cell>> m_cells;
};
} // namespace view

#endif //NINJACLOWN_VIEW_MAP_HPP
