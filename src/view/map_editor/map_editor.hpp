#ifndef NINJACLOWN_VIEW_MAP_EDITOR_MAP_EDITOR_HPP
#define NINJACLOWN_VIEW_MAP_EDITOR_MAP_EDITOR_HPP

#include <variant>
#include <optional>


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
	map_editor(sf::RenderWindow &window, state::holder &state)
	    : m_state{state}
	    , m_window{window} { }

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

	/**
	 * Loads a map or creates an empty one
	 */
	void initialize_map();

	/**
	 * Shows the selector (clickables to select what to place : tiles, mobs, ...)
	 */
	void show_selector();

	/**
	 * Displays the current selection under the cursor
	 */
	void display_selected();

	/**
	 * Displays the map being built
	 */
	void display_map();

	state::holder &m_state;
	sf::RenderWindow &m_window;

	bool m_has_map{false};
	std::size_t m_map_size_x{0};
	std::size_t m_map_size_y{0};

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