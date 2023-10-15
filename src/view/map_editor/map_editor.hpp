#ifndef NINJACLOWN_VIEW_MAP_EDITOR_MAP_EDITOR_HPP
#define NINJACLOWN_VIEW_MAP_EDITOR_MAP_EDITOR_HPP

#include    <variant>
#include <optional>

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
	 * Displays the map being built
	 */
	void display_map();

	/**
	 * Shows right click drop down menu
	 */
	 void display_popup();

	state::holder &m_state;
	sf::RenderWindow &m_window;

	editor_state m_editor_state{editor_state::showing_menu};
	int m_map_size_x{0}; // signed because of ImGui
	int m_map_size_y{0}; // signed because of ImGui
	bool m_popup_menu_open{false};

	bool m_has_map{false};
	file_explorer m_file_explorer{};

	bool m_stay_in_map_editor{true};

	std::optional<std::pair<int, int>> m_mouse_press_location{};

	/**
	 * Current selection. nullopt_t if no selection at all
	 */
	std::variant<std::nullopt_t,
	             utils::resources_type::mob_id,
	             utils::resources_type::object_id,
	             utils::resources_type::tile_id
	             > m_selection{std::nullopt};
};
} // namespace view
#endif //NINJACLOWN_VIEW_MAP_EDITOR_MAP_EDITOR_HPP
