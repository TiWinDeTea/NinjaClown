#ifndef NINJACLOWN_VIEW_MAP_EDITOR_MAP_EDITOR_HPP
#define NINJACLOWN_VIEW_MAP_EDITOR_MAP_EDITOR_HPP

#include <optional>
#include <variant>

#include "SFML/Window/Mouse.hpp"
#include "view/game/game_viewer.hpp"
#include "view/standalones/file_explorer.hpp"

namespace utils::resources_type {
enum class mob_id;
enum class object_id;
enum class tile_id;
} // namespace utils::resources_type

namespace sf {
class RenderWindow;
class Event;
} // namespace sf

namespace state {
class holder;
}

namespace view {
class map_editor {
public:
	map_editor(sf::RenderWindow &window, state::holder &state) noexcept;

	/**
	 * Displays the map editor.
	 * @return false if return to main menu was requested.
	 */
	bool show();

	/**
	 * Manages the event. Should be called by view when events are polled
	 */
	void event(sf::Event &);

	/**
	 * Sets the viewer map
	 */
	void set_map(map_viewer &&);

	map_viewer &get_map() {
		return m_map_viewer;
	}

private:
	enum class editor_state {
		showing_menu,
		loading_map,
		creating_map,
		editing_map,
	};

	/**
	 * Loads a map or creates an empty one (asks user)
	 */
	void display_map_creator();
	// actually loads the selected map
	void load_map();

	/**
	 * Shows the selector (clickables to select what to place : tiles, mobs, ...)
	 */
	void show_selector();
	void selector_tile();
	void selector_object();
	void selector_mob();

	/**
	 * Displays the current selection under the cursor
	 */
	void display_selected();

	/**
	 * Shows right click drop down menu
	 */
	void display_popup();

	/**
	 * Shows the field editor (accessed via rigth-clicking>edit)
	 */
	void display_field_editor();

	/**
	  * Adds an item to the map, sets a tile, .... according to user input
	  */
	void place_item();

	/**
	 * Computes the mouse position as in-map coordinates
	 * Returns an empty optional if the mouse isn’t hovering the map
	 */
	std::optional<sf::Vector2i> mouse_pos_as_map_pos();

	state::holder &m_state;
	sf::RenderWindow &m_window;

	editor_state m_editor_state{editor_state::showing_menu};
	int m_map_size_x{0}; // signed because of ImGui
	int m_map_size_y{0}; // signed because of ImGui

	bool m_popup_menu_open{false}; //! is the escape menu open?

	bool m_needs_open_right_click{false}; //! do we need to open the edit/delete/... context menu?

	bool m_has_map{false}; //! do we have a map currently being worked on?
	file_explorer m_file_explorer{};

	bool m_stay_in_map_editor{true}; //! should we stay in map editor or go back to main menu?

	std::optional<sf::Vector2<int>> m_mouse_press_location{}; //! Position of the last mouse press, if any
	std::optional<sf::Mouse::Button> m_mouse_press_button{}; //! Type of the last mouse press, if any
	std::optional<sf::Vector2<int>> m_last_placed_location{}; //! position of the last position where a tile/object/... was placed

	map_viewer m_map_viewer;

	/**
	 * Current selection. nullopt_t if no selection at all
	 */
	std::variant<std::nullopt_t, utils::resources_type::mob_id, utils::resources_type::object_id, utils::resources_type::tile_id>
	  m_selection{std::nullopt};

	// Used by right click pop-up menu
	std::optional<adapter::view_handle> m_hovered_entity{};
	adapter::entity_edit m_hovered_entity_fields{};
	bool m_editing_hovered_entity_fields{false}; //! Are we currently editing the fields ?
	bool m_field_editor_open{false};
};
} // namespace view
#endif //NINJACLOWN_VIEW_MAP_EDITOR_MAP_EDITOR_HPP
