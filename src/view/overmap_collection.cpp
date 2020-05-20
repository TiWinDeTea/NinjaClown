#include "view/overmap_collection.hpp"
#include "adapter/adapter.hpp"
#include "utils/resource_manager.hpp"
#include "utils/visitor.hpp"
#include "view/viewer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <spdlog/spdlog.h>

void view::overmap_collection::print_all(view::viewer &viewer) const noexcept {
	for (const auto &displayable : m_ordered_displayable) {
		displayable.first->vprint(viewer);
	}
}

void view::overmap_collection::print_all(view::viewer &viewer, adapter::adapter &adapter,
                                         utils::resource_manager &resources) const noexcept {
	for (const auto &displayable : m_ordered_displayable) {
		displayable.first->vprint(viewer);
		if (displayable.first->vis_hovered(viewer)) {
			utils::visitor request_visitor{[&](const adapter::request::hitbox &hitbox) {
				                               auto [screen_x, screen_y] = viewer.to_screen_coords(hitbox.x, hitbox.y);
				                               auto [screen_width, screen_height]
				                                 = viewer.to_screen_coords(hitbox.x + hitbox.width, hitbox.y + hitbox.height);
				                               screen_width -= screen_x;
				                               screen_height -= screen_y;

				                               sf::RectangleShape rect{{screen_width, screen_height}};
				                               rect.setPosition(screen_x, screen_y);
				                               rect.setFillColor(sf::Color{128, 255, 128, 128}); // todo externalize

				                               viewer.window->draw(rect);
			                               },
			                               [&](const adapter::request::coords_list &list) {
				                               for (const auto &coords : list.coords) {
					                               viewer.acquire_map()->highlight_tile(viewer, coords.x, coords.y, resources);
				                               }
			                               },
			                               [](std::monostate /* ignored */) {}};
			std::visit(request_visitor, adapter.tooltip_for(displayable.second));
		}
	}
}

adapter::view_handle view::overmap_collection::add_object(object &&obj) noexcept {
	adapter::view_handle handle{false, m_objects.size()};
	m_objects.emplace_back(std::move(obj));
	m_ordered_displayable.emplace(&m_objects.back(), handle);
	return handle;
}

adapter::view_handle view::overmap_collection::add_mob(mob &&mb) noexcept {
	adapter::view_handle handle{true, m_mobs.size()};
	m_mobs.emplace_back(std::move(mb));
	m_ordered_displayable.emplace(&m_mobs.back(), handle);
	return handle;
}

void view::overmap_collection::move_entity(adapter::view_handle handle, float newx, float newy) {
	auto it = std::find_if(m_ordered_displayable.begin(), m_ordered_displayable.end(), [handle](const auto &entity) {
		return entity.second == handle;
	});

	if (it == m_ordered_displayable.end()) {
		spdlog::error("Tried to move unknown view entity {{{} {}}}", handle.is_mob, handle.handle);
		return;
	}

	m_ordered_displayable.erase(it);
	auto target = std::next(m_mobs.begin(), handle.handle);
	if (handle.is_mob) {
		spdlog::trace("Moving view mob {} to ({} ; {})", handle.handle, newx, newy);
		target->set_pos(newx, newy);
		m_ordered_displayable.emplace(std::pair{&*target, handle});
	}
	else {
		spdlog::trace("Moving view object {} to ({} ; {})", handle.handle, newx, newy);
		target->set_pos(newx, newy);
		m_ordered_displayable.emplace(std::pair{&*target, handle});
	}
}

void view::overmap_collection::rotate_entity(adapter::view_handle handle, view::facing_direction::type new_direction) noexcept {
	if (!handle.is_mob) {
		spdlog::error("Rotate request for non-compatible view object {{{} {}}}", false, handle.handle);
		return;
	}

	std::next(m_mobs.begin(), handle.handle)->set_direction(new_direction);
}