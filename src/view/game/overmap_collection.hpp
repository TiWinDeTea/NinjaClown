#ifndef NINJACLOWN_VIEW_OVERMAP_COLLECTION_HPP
#define NINJACLOWN_VIEW_OVERMAP_COLLECTION_HPP

#include "mob.hpp"
#include "object.hpp"

#include "adapter/adapter.hpp"

#include <list>
#include <set>

namespace utils {
class resource_manager;
}

namespace view {
class map_viewer;

class overmap_collection {
	using pair_type = std::pair<const overmap_displayable_interface *, adapter::view_handle>;

	struct less {
		bool operator()(const pair_type &rhs, const pair_type &lhs) const noexcept {
			return *rhs.first < *lhs.first;
		}
	};

public:
	void reload_sprites() noexcept;

	void print_all(map_viewer &) const noexcept;

	[[nodiscard]] std::vector<std::vector<std::string>> print_all(map_viewer &, adapter::adapter &) const noexcept;

	adapter::view_handle add_object(object &&) noexcept;
	adapter::view_handle add_mob(mob &&) noexcept;

	void move_entity(adapter::view_handle handle, float newx, float newy);

	void rotate_entity(adapter::view_handle handle, facing_direction::type new_direction) noexcept;

	void hide(adapter::view_handle handle);
	void reveal(adapter::view_handle handle);

	void clear() noexcept;

	[[nodiscard]] const std::list<std::optional<mob>>& mobs() const noexcept {
		return m_mobs;
	}

	[[nodiscard]] const std::list<std::optional<object>>& objects() const noexcept {
		return m_objects;
	}

	/**
	 * returns the handle ptr-corresponding to the passed reference
	 */
	[[nodiscard]] std::optional<adapter::view_handle> get_handle(const mob& ptr) const noexcept;
	[[nodiscard]] std::optional<adapter::view_handle> get_handle(const object& ptr) const noexcept;

	// Deletes a mob or an item
	void erase(adapter::view_handle) noexcept;

	void set_mob_kind(adapter::view_handle, utils::resources_type::mob_id type);

private:

	std::multiset<pair_type, less> m_ordered_displayable;

	std::list<std::optional<mob>> m_mobs;
	std::list<std::optional<object>> m_objects;
};
} // namespace view

#endif //NINJACLOWN_VIEW_OVERMAP_COLLECTION_HPP
