
#include "view/overmap_collection.hpp"
#include "adapter/adapter.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"
#include "utils/visitor.hpp"
#include "view/map_viewer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <spdlog/spdlog.h>

using fmt::literals::operator""_a;

void view::overmap_collection::reload_sprites(const utils::resource_manager &res) noexcept {
	for (mob &mob : m_mobs) {
		mob.reload_sprites(res);
	}
	for (object &object : m_objects) {
		object.reload_sprites(res);
	}
}

void view::overmap_collection::clear() noexcept {
	m_ordered_displayable.clear();
	m_mobs.clear();
	m_objects.clear();
}

void view::overmap_collection::print_all(view::map_viewer &viewer) const noexcept {
	for (const auto &displayable : m_ordered_displayable) {
		displayable.first->print(viewer);
	}
}

std::vector<std::vector<std::string>> view::overmap_collection::print_all(view::map_viewer &viewer, adapter::adapter &adapter,
                                                                          utils::resource_manager &resources) const noexcept {

	std::vector<std::string> local_info;
	utils::visitor request_visitor{[&](const adapter::request::hitbox &hitbox) {
		                               auto [screen_x, screen_y] = viewer.to_screen_coords(hitbox.x, hitbox.y);
		                               auto [screen_width, screen_height]
		                                 = viewer.to_screen_coords(hitbox.x + hitbox.width, hitbox.y + hitbox.height);
		                               screen_width -= screen_x;
		                               screen_height -= screen_y;

		                               sf::RectangleShape rect{{screen_width, screen_height}};
		                               rect.setPosition(screen_x, screen_y);
		                               rect.setFillColor(sf::Color{128, 255, 128, 128}); // todo externalize

		                               viewer.draw(rect);
	                               },
	                               [&](const adapter::request::coords &coord) {
		                               viewer.highlight_tile({static_cast<int>(coord.x), static_cast<int>(coord.y)});
	                               },
	                               [&](const adapter::request::info &info) {
		                               std::move(info.lines.begin(), info.lines.end(), std::back_inserter(local_info));
	                               }};

	std::vector<std::vector<std::string>> ret;
	for (const auto &displayable : m_ordered_displayable) {
		displayable.first->print(viewer);
		if (displayable.first->is_hovered(viewer)) {
			local_info.clear();
			adapter::draw_request requests = adapter.tooltip_for(displayable.second);
			for (const adapter::draw_request::value_type &request : requests) {
				std::visit(request_visitor, request);
			}
			if (!local_info.empty()) {
				ret.emplace_back(std::move(local_info));
			}
		}
	}
	return ret;
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

void view::overmap_collection::move_entity(utils::resource_manager &res, adapter::view_handle handle, float newx, float newy) {
	auto it = std::find_if(m_ordered_displayable.begin(), m_ordered_displayable.end(), [handle](const auto &entity) {
		return entity.second == handle;
	});

	if (it == m_ordered_displayable.end()) {
		utils::log::error(res, "overmap_collection.unknown_entity_move", "is_mob"_a = handle.is_mob, "handle"_a = handle.handle);
		spdlog::error("Tried to move unknown view entity {{{} {}}}", handle.is_mob, handle.handle);
		return;
	}

	m_ordered_displayable.erase(it);
	auto target = std::next(m_mobs.begin(), handle.handle);
	if (handle.is_mob) {
		utils::log::trace(res, "overmap_collection.moving_mob", "handle"_a = handle.handle, "x"_a = newx, "y"_a = newy);
		target->set_pos(newx, newy);
		m_ordered_displayable.emplace(std::pair{&*target, handle});
	}
	else {
		utils::log::trace(res, "overmap_collection.moving_object", "handle"_a = handle.handle, "x"_a = newx, "y"_a = newy);
		target->set_pos(newx, newy);
		m_ordered_displayable.emplace(std::pair{&*target, handle});
	}
}

void view::overmap_collection::rotate_entity(utils::resource_manager &res, adapter::view_handle handle,
                                             view::facing_direction::type new_direction) noexcept {
	if (!handle.is_mob) {
		utils::log::error(res, "overmap_collection.incompatible_rotate", "handle"_a = handle.handle);
		return;
	}

	std::next(m_mobs.begin(), handle.handle)->set_direction(new_direction);
}

void view::overmap_collection::hide(adapter::view_handle handle) {
	if (handle.is_mob) {
		std::next(m_mobs.begin(), handle.handle)->hide();
	}
	else {
		std::next(m_objects.begin(), handle.handle)->hide();
	}
}

void view::overmap_collection::reveal(adapter::view_handle handle) {
	if (handle.is_mob) {
		std::next(m_mobs.begin(), handle.handle)->reveal();
	}
	else {
		std::next(m_objects.begin(), handle.handle)->reveal();
	}
}
