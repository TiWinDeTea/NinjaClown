#include "map_editor.hpp"
#include "utils/resource_manager.hpp"
#include "view/standalones/imgui_styles.hpp"
#include "state_holder.hpp"
#include "adapter/adapter.hpp"

#include <imgui.h>

#include <SFML/Window/Event.hpp>

constexpr const char * popup_name = "##view.map_editor.map_editor.display_map_creator";

bool view::map_editor::show() {
	switch (m_editor_state) {
		case editor_state::showing_menu:
			// fallthrough
		case editor_state::creating_map:
			display_map_creator();
			break;
		case editor_state::loading_map:
			if (m_file_explorer.path_ready()) {
				load_map();
				m_file_explorer.close();
			} else {
				m_file_explorer.give_control();
				if (!m_file_explorer.showing()) {
					m_editor_state = editor_state::showing_menu;
				}
			}
			break;
		case editor_state::editing_map:
			display_map();
			show_selector();
			display_selected();
			break;
	}

	return true;
}

void view::map_editor::event(sf::Event & event) {
	// TODO
	if (event.type == sf::Event::KeyPressed) {

		if (event.key.code == sf::Keyboard::Escape) {

			if (m_editor_state != editor_state::showing_menu) {
				m_editor_state = editor_state::showing_menu;
			} else if (m_has_map) {
				m_editor_state = editor_state::editing_map;
			}

		}
	}
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
void view::map_editor::display_map_creator() {
	const auto& res = utils::resource_manager::instance();
	auto& adapter = state::access<map_editor>::adapter(m_state);

	if (!m_popup_open) {
		ImGui::OpenPopup(popup_name);
		m_popup_open = true;
	}

	if (ImGui::BeginPopupModal(popup_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (m_editor_state == editor_state::creating_map) {
			ImGui::SliderInt(res.gui_text_for("view.map_editor.map_size_x").data(), &m_map_size_x, 0, 200);
			ImGui::SliderInt(res.gui_text_for("view.map_editor.map_size_y").data(), &m_map_size_y, 0, 200);

			if (m_map_size_x <= 3 || m_map_size_y <= 3 || m_map_size_x * m_map_size_y <= 20) {
				using_style(disabled_button) {
					ImGui::Button(res.gui_text_for("view.map_editor.generate_map").data());
				};
			}
			else {
				if (ImGui::Button(res.gui_text_for("view.map_editor.generate_map").data())) {
					adapter.create_map(static_cast<unsigned>(m_map_size_x), static_cast<unsigned>(m_map_size_y));
					m_has_map = true;
					m_editor_state = editor_state::editing_map;
					ImGui::CloseCurrentPopup();
					m_popup_open = false;
				}
			}

			if (ImGui::Button(res.gui_text_for("view.map_editor.cancel_map_creation").data())) {
				m_editor_state = editor_state::showing_menu;
				ImGui::CloseCurrentPopup();
				m_popup_open = false;
			}

		} else {
			if (ImGui::Button(res.gui_text_for("view.map_editor.load_map").data())) {
				m_editor_state = editor_state::loading_map;
				ImGui::CloseCurrentPopup();
				m_popup_open = false;
				m_file_explorer.open(with_extensions{".map"}); // TODO externalize ".map" (check for other occurences)

			} else if (ImGui::Button(res.gui_text_for("view.map_editor.create_map").data())) {
				m_editor_state = editor_state::creating_map;
			}
		}
		ImGui::EndPopup();
	}
}

void view::map_editor::load_map() {
	// TODO : load map with m_file_explorer.selected_path();
	m_has_map = true;
}
