
#include "adapter/adapter.hpp"
#include "overmap_collection.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"
#include "utils/visitor.hpp"
#include "view/game/map_viewer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <spdlog/spdlog.h>

using fmt::literals::operator""_a;

void view::overmap_collection::reload_sprites() noexcept {
	for (std::optional<mob> &mob : m_mobs) {
		if (mob) {
			mob->reload_sprites();
		}
	}
	for (std::optional<object> &object : m_objects) {
		if (object) {
			object->reload_sprites();
		}
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

std::vector<std::vector<std::string>> view::overmap_collection::print_all(view::map_viewer &viewer,
                                                                          adapter::adapter &adapter) const noexcept {

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
		if (displayable.first->is_hovered(viewer)) {
			local_info.clear();
			const adapter::draw_request requests = adapter.tooltip_for(displayable.second);
			for (const adapter::draw_request::value_type &request : requests) {
				std::visit(request_visitor, request);
			}
			if (!local_info.empty()) {
				ret.emplace_back(std::move(local_info));
			}
		}
	}
	for (const auto& displayable : m_ordered_displayable) {
		displayable.first->print(viewer);
	}
	return ret;
}

adapter::view_handle view::overmap_collection::add_object(object &&obj) noexcept {

	// scanning for first empty slot
	unsigned int handle_id = 0;
	auto it = m_objects.begin();
	for (; it != m_objects.end() ; ++it, ++handle_id) {
		// empty optional : it’s our new space
		if (!*it) {
			break;
		}
	}

    adapter::view_handle handle{false, handle_id};
	if (it == m_objects.end()) {
		m_objects.emplace_back(std::move(obj));
		m_ordered_displayable.emplace(&*m_objects.back(), handle);
	} else {
		(*it).emplace(std::move(obj));
		m_ordered_displayable.emplace(&**it, handle);
	}

	return handle;
}

adapter::view_handle view::overmap_collection::add_mob(mob &&mb) noexcept {

	// scanning for first empty slot
	unsigned int handle_id = 0;
	auto it = m_mobs.begin();
	for (; it != m_mobs.end() ; ++it, ++handle_id) {
		// empty optional : it’s our new space
		if (!*it) {
			break;
		}
	}

    adapter::view_handle handle{true, handle_id};
	if (it == m_mobs.end()) {
		m_mobs.emplace_back(std::move(mb));
		m_ordered_displayable.emplace(&*m_mobs.back(), handle);
	} else {
		(*it).emplace(std::move(mb));
		m_ordered_displayable.emplace(&**it, handle);
	}

	return handle;
}

void view::overmap_collection::move_entity(adapter::view_handle handle, float newx, float newy) {
	const auto& res = utils::resource_manager::instance();

	auto it = std::find_if(m_ordered_displayable.begin(), m_ordered_displayable.end(), [handle](const auto &entity) {
		return entity.second == handle;
	});

	if (it == m_ordered_displayable.end()) {
		utils::log::error("overmap_collection.unknown_entity_move", "is_mob"_a = handle.is_mob, "handle"_a = handle.handle);
		return;
	}

	m_ordered_displayable.erase(it);
	if (handle.is_mob) {
		assert(handle.handle < m_mobs.size());
		auto target = std::next(m_mobs.begin(), handle.handle);

		if (!*target) {
			utils::log::error("overmap_collection.invalid_mob_handle_move", "handle"_a = handle.handle);
			return;
		}

		(*target)->set_pos(newx, newy);
		m_ordered_displayable.emplace(std::pair{&**target, handle});

		utils::log::trace( "overmap_collection.moving_mob", "handle"_a = handle.handle, "x"_a = newx, "y"_a = newy);

	}
	else {
		assert(handle.handle < m_objects.size());
		auto target = std::next(m_objects.begin(), handle.handle);

		if (!*target) {
			utils::log::error("overmap_collection.invalid_object_handle_move", "handle"_a = handle.handle);
			return;
		}

		(*target)->set_pos(newx, newy);
		m_ordered_displayable.emplace(std::pair{&**target, handle});

		utils::log::trace("overmap_collection.moving_object", "handle"_a = handle.handle, "x"_a = newx, "y"_a = newy);
	}
}

void view::overmap_collection::rotate_entity(adapter::view_handle handle,
                                             view::facing_direction::type new_direction) noexcept {
	if (!handle.is_mob) {
		utils::log::error("overmap_collection.incompatible_rotate", "handle"_a = handle.handle);
		return;
	}

	assert(handle.handle < m_mobs.size());
	auto& mob = *std::next(m_mobs.begin(), handle.handle);
	if (mob) {
		mob->set_direction(new_direction);
	} else {
		utils::log::error("overmap_collection.invalid_mob_handle_rotate", "handle"_a = handle.handle);
	}
}

void view::overmap_collection::hide(adapter::view_handle handle) {
	if (handle.is_mob) {
		auto& mob = *std::next(m_mobs.begin(), handle.handle);
		assert(mob);
		mob->hide();
	}
	else {
		auto& object = *std::next(m_objects.begin(), handle.handle);
		assert(object);
		object->hide();
	}
}

void view::overmap_collection::reveal(adapter::view_handle handle) {
	if (handle.is_mob) {
		auto& mob = *std::next(m_mobs.begin(), handle.handle);
		assert(mob);
		mob->reveal();
	}
	else {
		auto& object = *std::next(m_objects.begin(), handle.handle);
		assert(object);
		object->reveal();
	}
}


std::optional<adapter::view_handle> view::overmap_collection::get_handle(const mob& ptr) const noexcept {
	unsigned int handle = 0;
	for (auto it = m_mobs.begin() ; handle < m_mobs.size() ; ++handle, ++it) {
		// if not empty optional and ptr corresponds
		if (*it && &**it == &ptr) {
			return adapter::view_handle{true, handle};
		}
	}
	return {};
}

std::optional<adapter::view_handle> view::overmap_collection::get_handle(const object& ptr) const noexcept {
	unsigned int handle = 0;
	for (auto it = m_objects.begin() ; handle < m_objects.size() ; ++handle, ++it) {
		// if not empty optional and ptr corresponds
		if (*it && &**it == &ptr) {
			return adapter::view_handle{false, handle};
		}
	}
	return {};
}

void view::overmap_collection::erase(adapter::view_handle handle) noexcept {
	auto it = std::find_if(m_ordered_displayable.begin(), m_ordered_displayable.end(), [handle](const auto &entity) {
		return entity.second == handle;
	});

	// if invalid handle
	if (it == m_ordered_displayable.end()) {
		return;
	}

	m_ordered_displayable.erase(it);

	if (handle.is_mob) {
		assert(handle.handle < m_mobs.size());
		std::next(m_mobs.begin(), handle.handle)->reset();
	} else {
		assert(handle.handle < m_objects.size());
		std::next(m_objects.begin(), handle.handle)->reset();
	}
}

void view::overmap_collection::set_mob_kind(adapter::view_handle handle, utils::resources_type::mob_id type) {
	assert(handle.handle < m_mobs.size());
	auto& target = *std::next(m_mobs.begin(), handle.handle);

	if (!target || !handle.is_mob) {
		utils::log::error("overmap_collection.bad_handle", "handle"_a = handle.handle, "is_mob"_a = handle.is_mob);
		return;
	}

	target->set_mob_id(type);
}
