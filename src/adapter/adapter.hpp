#ifndef NINJACLOWN_ADAPTER_ADAPTER_HPP
#define NINJACLOWN_ADAPTER_ADAPTER_HPP

#include <filesystem>
#include <set>
#include <unordered_map>
#include <variant>
#include <vector>

#include "model/grid_point.hpp"
#include "utils/utils.hpp"

#include "adapter_classes.hpp"

class terminal_commands;

namespace model {
enum class cell_type;
struct components;
}

namespace state {
class holder;
}

namespace cpptoml {
class table;
}

namespace utils::resources_type {
enum class tile_id;
enum class mob_id;
enum class object_id;
}

namespace adapter {


using draw_request = std::vector<std::variant<request::coords, request::hitbox, request::info>>;

class adapter {
public:
	explicit adapter(state::holder *state_holder) noexcept
	    : m_state{*state_holder} { }

	adapter(const adapter&) = delete;

	// -- Used by view -- //

	bool load_map(const std::filesystem::path &path) noexcept;

	bool map_is_loaded() noexcept;

	[[nodiscard]] draw_request tooltip_for(view_handle entity) noexcept;

	// Used by view when map editing
	void create_map(unsigned int width, unsigned int height);
	void edit_tile(unsigned int x, unsigned int y, utils::resources_type::tile_id new_value);
	void add_mob(unsigned int x, unsigned int y, utils::resources_type::mob_id id);
	void add_object(unsigned int x, unsigned int y, utils::resources_type::object_id id);
	void remove_entity(view_handle deleted_entity);
	bool can_be_toggled(view_handle handle);
	void toggle(view_handle handle);

	/**
	 * @param handle Handle to the view item
	 * @return Returns the list of available properties for a given item (may be empty)
	 */
	[[nodiscard]] entity_edit::edits entity_properties(view_handle handle) const;
	void edit_entity(view_handle handle, const entity_edit::edits & new_data);

	// -- Used by model -- //

	void fire_activator(model_handle handle) noexcept;

	void close_gate(model_handle gate) noexcept;
	void open_gate(model_handle gate) noexcept;

	void update_map(const model::grid_point &target, model::cell_type new_cell) noexcept;
	void clear_cells_changed_since_last_update() noexcept;

	void move_entity(model_handle entity, float new_x, float new_y) noexcept;
	void hide_entity(model_handle entity) noexcept;
	void rotate_entity(model_handle entity, float new_rad) noexcept;
	/**
	 * "Touch" this entity so that next call to `entities_changed_since_last_update` returns it.
	 */
	void mark_entity_as_dirty(model::handle_t) noexcept;
	void clear_entities_changed_since_last_update() noexcept;

	// -- Used by bot / dll -- //

	const std::vector<model::grid_point> &cells_changed_since_last_update() noexcept;
	const std::vector<std::size_t> &entities_changed_since_last_update() noexcept;

	void bot_log(bot_log_level level, const char *text);

private:
	/**
	 * Generates a tooltip for an actionable (door, ...), given its handle
	 * @param actionable Model handle to the actionable
	 * @param view_actionable View handle to the actionable
	 * @return Draw request for the actionable's tooltip
	 */
	[[nodiscard]] draw_request tooltip_for_actionable(model_handle actionable, view_handle view_actionable) noexcept;
	/**
	 * Generates a tooltip for an activator (button, ...), given its handle
	 * @param activator Model handle to the activator
	 * @return Draw request for the activator's tooltip
	 */
	[[nodiscard]] draw_request tooltip_for_activator(model_handle activator) noexcept;
	/**
	 * Generates a tooltip for a mob, given its handle
	 * @param mob Model handle to the mob
	 * @param components Model components
	 * @return Draw request for the mob's tooltip
	 */
	[[nodiscard]] draw_request tooltip_for_mob(model_handle mob, const model::components& components) noexcept;

	/**
	 * Compute the list of properties that can be edited (used by map editor)
	 */
	[[nodiscard]] entity_edit::edits actionable_entity_properties(view_handle vhandle, model_handle mhandle) const;
	[[nodiscard]] entity_edit::edits activator_entity_properties(view_handle vhandle, model_handle mhandle) const;
	[[nodiscard]] entity_edit::edits mob_entity_properties(view_handle vhandle, model_handle mhandle) const;

	/**
	 * Enacts modifications of properties (used by map editor)
	 */
	void edit_actionable_entity(view_handle handle, const entity_edit::edits& edits);
	void edit_activator_entity(view_handle handle, const entity_edit::edits& edits);
	void edit_mob_entity(view_handle handle, const entity_edit::edits& edits);

	friend terminal_commands;

	bool load_map_v1_0_0(const std::shared_ptr<cpptoml::table> &tables, std::string_view map) noexcept;

	state::holder &m_state;

	std::optional<view_handle> m_target_handle{}; //! handle to the objective (end of level) block
	std::unordered_map<model_handle, view_handle, model_hhash> m_model2view;
	std::unordered_map<view_handle, model_handle, view_hhash> m_view2model;
	std::unordered_map<view_handle, std::string, view_hhash> m_view2name;
	std::unordered_map<std::string, view_handle> m_name2view; // FIXME : ensure name unicity when editing a map

	std::vector<model::grid_point> m_cells_changed_since_last_update{};
	std::vector<std::size_t> m_entities_changed_since_last_update{};
};
} // namespace adapter

#endif //NINJACLOWN_ADAPTER_ADAPTER_HPP
