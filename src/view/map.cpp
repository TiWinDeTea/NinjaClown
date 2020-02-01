#include "view/map.hpp"
#include "view/viewer.hpp"

#include "utils/optional.hpp"
#include "utils/resource_manager.hpp"

void view::map::print(view::viewer &view, utils::resource_manager &resources) const noexcept {
	static_assert(static_cast<int>(cell::iron_tile) == 0);
	static_assert(static_cast<int>(cell::concrete_tile) == 1);
	static_assert(static_cast<int>(cell::abyss) == 2);

	std::array<utils::optional<const view::animation &>, 3> animations{resources.tile_animation(utils::resource_manager::tile_id::iron),
	                                                                   resources.tile_animation(utils::resource_manager::tile_id::concrete),
	                                                                   resources.tile_animation(utils::resource_manager::tile_id::chasm)};

	for (unsigned int x = 0; x < m_cells.size(); ++x) {
		for (unsigned int y = 0; y < m_cells[x].size(); ++y) {
			auto value = static_cast<int>(m_cells[x][y]);
			assert(animations[value]);
			animations[value]->print(view, static_cast<float>(x), static_cast<float>(y));
		}
	}
}

void view::map::highlight_tile(view::viewer &view, size_t x, size_t y, utils::resource_manager &resources) const noexcept {
	utils::optional<const view::animation &> anim;
	switch (m_cells[x][y]) {
		case cell::iron_tile:
			anim = resources.tile_animation(utils::resource_manager::tile_id::iron);
			break;
		case cell::concrete_tile:
			anim = resources.tile_animation(utils::resource_manager::tile_id::concrete);
			break;
		case cell::abyss:
			break;
	}

	if (anim) {
		anim->highlight(view, static_cast<float>(x), static_cast<float>(y));
	}
	frame_tile(view, x, y, resources);
}

void view::map::frame_tile(view::viewer &view, size_t x, size_t y, utils::resource_manager &resources) const noexcept {
	auto animation = resources.tile_animation(utils::resource_manager::tile_id::frame);
	if (animation) {
		animation->print(view, static_cast<float>(x), static_cast<float>(y));
	}
}
