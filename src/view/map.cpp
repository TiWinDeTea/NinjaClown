#include "view/map.hpp"

#include "program_state.hpp"
#include "utils/resource_manager.hpp"
#include "utils/optional.hpp"

void view::map::print(sf::RenderWindow &window) const noexcept {
    static_assert(static_cast<int>(cell::iron_tile) == 0);
    static_assert(static_cast<int>(cell::concrete_tile) == 1);
    static_assert(static_cast<int>(cell::abyss) == 2);

    auto& rm = program_state::global->resource_manager;
    std::array<utils::optional<const view::animation&>, 3> animations {
        rm.tile_animation(utils::resource_manager::tile_id::iron),
        rm.tile_animation(utils::resource_manager::tile_id::concrete),
        rm.tile_animation(utils::resource_manager::tile_id::chasm)
    };

    for (unsigned int x = 0 ; x < m_cells.size() ; ++x) {
        for (unsigned int y = 0 ; y < m_cells[x].size() ; ++y) {
            auto value = static_cast<int>(m_cells[x][y]);
            assert(animations[value]);
            animations[value]->print(window, static_cast<int>(x), static_cast<int>(y));
        }
    }
}
