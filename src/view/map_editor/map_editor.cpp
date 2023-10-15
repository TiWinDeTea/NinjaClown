#include "map_editor.hpp"
#include "adapter/adapter.hpp"
#include "state_holder.hpp"
#include "utils/resource_manager.hpp"
#include "utils/resources_type.hpp"
#include "utils/visitor.hpp"
#include "view/standalones/imgui_styles.hpp"

#include <imgui-SFML.h>
#include <imgui.h>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <atomic>

namespace {
std::atomic_bool globtooltip_popup_was_opened = false;
const char* text_for(std::string_view key) {
	return utils::resource_manager::instance().gui_text_for(key).data();
}
}

constexpr const char *popup_menu_name       = "##view.map_editor.map_editor.display_map_creator";
constexpr const char *selector_name         = "##view.map_editor.map_editor.show_selector";
constexpr const char *right_click_menu_name = "##view.map_editor.map_editor.display_popup";
constexpr const char *tooltip_popup         = "##view.map_editor.map_editor.tooltip_popup";

view::map_editor::map_editor(sf::RenderWindow &window, state::holder &state) noexcept
    : m_state{state}
    , m_window{window} { }

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
			}
			else {
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
			display_popup(); // if needed (managed by ImGui)
			break;
	}

	return std::exchange(m_stay_in_map_editor, true);
}

void view::map_editor::event(sf::Event &event) {
	if (event.type == sf::Event::KeyPressed) {

		if (event.key.code == sf::Keyboard::Escape) {

			if (m_editor_state != editor_state::showing_menu) {
				m_editor_state = editor_state::showing_menu;
			}
			else if (m_has_map) {
				m_editor_state = editor_state::editing_map;
				if (ImGui::BeginPopupModal(popup_menu_name)) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
				}
				m_popup_menu_open = false;
			}
		}

		return;
	}

	if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Button::Right) {
		m_mouse_press_location = {event.mouseButton.x, event.mouseButton.y};
		return;
	}

	if (event.type == sf::Event::MouseButtonReleased) {
		if (!m_mouse_press_location) {
			return;
		}
		if (m_mouse_press_location->first != event.mouseButton.x || m_mouse_press_location->second != event.mouseButton.y) {
			return;
		}

		ImGui::OpenPopup(right_click_menu_name);
	}
}

void view::map_editor::show_selector() {
	if (ImGui::Begin(selector_name)) {

		selector_tile();
		ImGui::Separator();
		selector_object();
		ImGui::Separator();
		selector_mob();
	}
	ImGui::End();
}

void view::map_editor::display_selected() {
	const sf::Vector2i mpos = sf::Mouse::getPosition(m_window);
	const sf::Vector2f mpos_f{static_cast<float>(mpos.x), static_cast<float>(mpos.y)};

	utils::visitor visitor {
		[](std::nullopt_t) {},
		[this, &mpos_f](utils::resources_type::mob_id id) {
			  auto sprite = utils::resource_manager::instance().mob_animations(id)->current_sprite();
			  sprite.setPosition(mpos_f);
			  m_window.draw(sprite);
		  },
		[this, &mpos_f](utils::resources_type::object_id id) {
			  auto sprite = utils::resource_manager::instance().object_animation(id)->current_sprite();
			  sprite.setPosition(mpos_f);
			  m_window.draw(sprite);

		  },
		[this, &mpos_f](utils::resources_type::tile_id id) {
			  auto sprite = utils::resource_manager::instance().tile_animation(id)->current_sprite();
			  sprite.setPosition(mpos_f);
			  m_window.draw(sprite);
		  },

	};

	std::visit(visitor, m_selection);
}

void view::map_editor::display_map() {
	// TODO
}

void view::map_editor::display_map_creator() {
	const auto &res = utils::resource_manager::instance();
	auto text_for   = [&](const char *key) {
        return res.gui_text_for(key).data();
	};
	auto &adapter = state::access<map_editor>::adapter(m_state);

	if (!m_popup_menu_open) {
		ImGui::OpenPopup(popup_menu_name);
		m_popup_menu_open = true;
	}

	if (ImGui::BeginPopupModal(popup_menu_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (m_editor_state == editor_state::creating_map) {
			ImGui::SliderInt(text_for("view.map_editor.map_size_x"), &m_map_size_x, 0, 200);
			ImGui::SliderInt(text_for("view.map_editor.map_size_y"), &m_map_size_y, 0, 200);

			if (m_map_size_x <= 3 || m_map_size_y <= 3 || m_map_size_x * m_map_size_y <= 20) {
				using_style(disabled_button) {
					ImGui::Button(text_for("view.map_editor.generate_map"));
				};
			}
			else {
				if (ImGui::Button(text_for("view.map_editor.generate_map"))) {
					adapter.create_map(static_cast<unsigned>(m_map_size_x), static_cast<unsigned>(m_map_size_y));
					m_has_map      = true;
					m_editor_state = editor_state::editing_map;
					ImGui::CloseCurrentPopup();
					m_popup_menu_open = false;
				}
			}

			if (ImGui::Button(text_for("view.map_editor.cancel_map_creation"))) {
				m_editor_state = editor_state::showing_menu;
				ImGui::CloseCurrentPopup();
				m_popup_menu_open = false;
			}
		}
		else {
			if (ImGui::Button(res.gui_text_for("view.map_editor.load_map").data())) {
				m_editor_state = editor_state::loading_map;
				ImGui::CloseCurrentPopup();
				m_popup_menu_open = false;
				m_file_explorer.open(with_extensions{".map"}); // TODO externalize ".map" (check for other occurences)
			}
			else if (ImGui::Button(res.gui_text_for("view.map_editor.create_map").data())) {
				m_editor_state = editor_state::creating_map;
			}

			if (ImGui::Button(res.gui_text_for("view.in_game_menu.return_to_main_menu").data())) {
				m_stay_in_map_editor = false;
			}
		}

		ImGui::EndPopup();
	}
}

void view::map_editor::load_map() {
	// TODO : load map with m_file_explorer.selected_path();
	m_has_map = true;
}

void view::map_editor::display_popup() {

	// TODO : fill stub
	if (ImGui::BeginPopup(right_click_menu_name)) {
		if (ImGui::Selectable("delete")) {
		}
		if (ImGui::Selectable("edit")) {
		}
		if (ImGui::Selectable("link to (for buttons)")) {
		}
		ImGui::EndPopup();
	}
}
void view::map_editor::selector_tile() {
	const std::array tiles_type = {
	  std::pair{utils::resources_type::tile_id::iron, text_for("view.map_editor.tiles.iron")},
	  std::pair{utils::resources_type::tile_id::chasm, text_for("view.map_editor.tiles.chasm")},
	  std::pair{utils::resources_type::tile_id::concrete, text_for("view.map_editor.tiles.concrete")},
	};

	for (auto tile : tiles_type) {
		ImGui::ImageButton(utils::resource_manager::instance().tile_animation(tile.first)->current_sprite(), {32, 32});
		if (ImGui::IsItemClicked()) { // Doing it this way because the normal way ain’t working with our version of ImGui
			m_selection = tile.first;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", tile.second);
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();
}

void view::map_editor::selector_object() {
	const std::array objects_type = {
	  std::pair{utils::resources_type::object_id::button, text_for("view.map_editor.objects.button")},
	  std::pair{utils::resources_type::object_id::gate, text_for("view.map_editor.objects.gate")},
	  std::pair{utils::resources_type::object_id::autoshooter, text_for("view.map_editor.objects.autoshooter")}, // Sprite does not exist yet on ISO (crashes)
	  std::pair{utils::resources_type::object_id::target, text_for("view.map_editor.objects.target")},
	};


	for (auto object : objects_type) {
		ImGui::ImageButton(utils::resource_manager::instance().object_animation(object.first)->current_sprite(), {32, 32});
		if (ImGui::IsItemClicked()) { // Doing it this way because the normal way ain’t working with our version of ImGui
			m_selection = object.first;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", object.second);
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();

}
void view::map_editor::selector_mob() {
	const std::array mobs_type = {
	  std::pair{utils::resources_type::mob_id::player, text_for("view.map_editor.mobs.player")},
	  std::pair{utils::resources_type::mob_id::scientist, text_for("view.map_editor.mobs.scientist")},
	};

	for (auto mob : mobs_type) {
		ImGui::ImageButton(utils::resource_manager::instance().mob_animations(mob.first)->current_sprite(), {32, 32});
		if (ImGui::IsItemClicked()) { // Doing it this way because the normal way ain’t working with our version of ImGui
			m_selection = mob.first;
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", mob.second);
		}
		ImGui::SameLine();
	}
	ImGui::NewLine();
}
