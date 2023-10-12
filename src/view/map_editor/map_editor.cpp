#include "map_editor.hpp"

bool view::map_editor::show() {
	if (m_has_map) {
		display_map();
		show_selector();
		display_selected();

	}
	else {
		initialize_map();
	}

	return true;
}

void view::map_editor::event(sf::Event &) {
	// TODO
}

void view::map_editor::show_selector() {
	// TODO
}

void view::map_editor::display_selected() {
	// TODO
}

void view::map_editor::display_map() {
	// TODO
}
void view::map_editor::initialize_map() {
	// TODO
}
