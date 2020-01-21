#include "view/map.hpp"

#include "program_state.hpp"
#include "utils/optional.hpp"
#include "utils/resource_manager.hpp"

void view::map::print(sf::RenderWindow &window) const noexcept {
	static_assert(static_cast<int>(cell::iron_tile) == 0);
	static_assert(static_cast<int>(cell::concrete_tile) == 1);
	static_assert(static_cast<int>(cell::abyss) == 2);

	auto &rm = program_state::global->resource_manager;
	std::array<utils::optional<const view::animation &>, 3> animations{rm.tile_animation(utils::resource_manager::tile_id::iron),
	                                                                   rm.tile_animation(utils::resource_manager::tile_id::concrete),
	                                                                   rm.tile_animation(utils::resource_manager::tile_id::chasm)};

	for (unsigned int x = 0; x < m_cells.size(); ++x) {
		for (unsigned int y = 0; y < m_cells[x].size(); ++y) {
			auto value = static_cast<int>(m_cells[x][y]);
			assert(animations[value]);
			animations[value]->print(window, static_cast<float>(x), static_cast<float>(y));
		}
	}
}

void view::map::highlight_tile(sf::RenderWindow &window, size_t x, size_t y) const noexcept {
	auto &rm = program_state::global->resource_manager;

	utils::optional<const view::animation &> anim;
	switch (m_cells[x][y]) {
		case cell::iron_tile:
			anim = rm.tile_animation(utils::resource_manager::tile_id::iron);
			break;
		case cell::concrete_tile:
			anim = rm.tile_animation(utils::resource_manager::tile_id::concrete);
			break;
		case cell::abyss:
			break;
	}

	if (anim) {
		anim->highlight(window, static_cast<float>(x), static_cast<float>(y));
	}
	frame_tile(window, x, y);
}

void view::map::frame_tile(sf::RenderWindow &window, size_t x, size_t y) const noexcept {
	auto animation = program_state::global->resource_manager.tile_animation(utils::resource_manager::tile_id::frame);
	if (animation) {
		animation->print(window, static_cast<float>(x), static_cast<float>(y));
	}
}
