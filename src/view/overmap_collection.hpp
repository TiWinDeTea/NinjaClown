#ifndef NINJACLOWN_VIEW_OVERMAP_COLLECTION_HPP
#define NINJACLOWN_VIEW_OVERMAP_COLLECTION_HPP

#include "view/mob.hpp"
#include "view/object.hpp"

#include "adapter/adapter.hpp"

#include <list>
#include <set>

namespace utils {
class resource_manager;
}

namespace view {
class viewer;

class overmap_collection {
	using pair_type = std::pair<const overmap_displayable_interface *, adapter::view_handle>;

	struct less {
		bool operator()(const pair_type &rhs, const pair_type &lhs) const noexcept {
			return *rhs.first < *lhs.first;
		}
	};

public:
	void reload_sprites(const utils::resource_manager &res) noexcept;

	void print_all(view::viewer &) const noexcept;

	[[nodiscard]] std::vector<std::vector<std::string>> print_all(view::viewer &, adapter::adapter &,
	                                                              utils::resource_manager &) const noexcept;

	adapter::view_handle add_object(object &&) noexcept;
	adapter::view_handle add_mob(mob &&) noexcept;

	void move_entity(utils::resource_manager &, adapter::view_handle handle, float newx, float newy);

	void rotate_entity(utils::resource_manager &, adapter::view_handle handle, view::facing_direction::type new_direction) noexcept;

	void hide(adapter::view_handle handle);
	void reveal(adapter::view_handle handle);

	void clear() noexcept;

private:
	std::multiset<pair_type, less> m_ordered_displayable;

	std::list<mob> m_mobs;
	std::list<object> m_objects;
};
} // namespace view

#endif //NINJACLOWN_VIEW_OVERMAP_COLLECTION_HPP
