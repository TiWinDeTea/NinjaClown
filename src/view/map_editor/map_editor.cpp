#include "map_editor.hpp"
#include "adapter/adapter.hpp"
#include "state_holder.hpp"
#include "utils/resource_manager.hpp"
#include "utils/resources_type.hpp"
#include "utils/universal_constants.hpp"
#include "utils/visitor.hpp"
#include "view/standalones/imgui_styles.hpp"
#include "view/standalones/mouse_position.hpp"

#include <imgui-SFML.h>
#include <imgui.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>

#include <atomic>

namespace {
const char *text_for(std::string_view key) {
	return utils::gui_text_for(key).data();
}
} // namespace

constexpr const char *popup_menu_name        = "##view.map_editor.map_editor.display_map_creator";
constexpr const char *selector_name          = "##view.map_editor.map_editor.show_selector";
constexpr const char *right_click_menu_name  = "##view.map_editor.map_editor.display_popup";
constexpr const char *field_editor_menu_name = "##map_editor.display_field_editor";

view::map_editor::map_editor(sf::RenderWindow &window, state::holder &state) noexcept
    : m_state{state}
    , m_window{window}
    , m_map_viewer(state) { }

bool view::map_editor::show() {

	if (m_has_map) {
		m_map_viewer.print(m_editor_state == editor_state::editing_map);
		show_selector();
	}

	switch (m_editor_state) {
		case editor_state::showing_menu:
			// fallthrough
		case editor_state::creating_map:
			display_map_creator();
			break;
		case editor_state::loading_map:
			m_file_explorer.give_control();
			if (!m_file_explorer.showing()) {
				if (m_file_explorer.path_ready()) {
					load_map();
					m_file_explorer.close();
					m_editor_state = editor_state::editing_map;
				}
				else {
					m_editor_state = editor_state::showing_menu;
				}
			}
			break;
		case editor_state::editing_map:
			display_selected();
			display_popup(); // if needed (managed by ImGui, cf ImGui::OpenPopup(right_click_menu_name))
			display_field_editor(); // if needed
			break;
	}

	return std::exchange(m_stay_in_map_editor, true);
}

void view::map_editor::event(sf::Event &event) {
	if (event.type == sf::Event::KeyPressed) {

		if (event.key.code == sf::Keyboard::Escape) {
			if (!std::holds_alternative<std::nullopt_t>(m_selection)) {
				m_selection = std::nullopt;
				return;
			}

			if (m_editor_state != editor_state::showing_menu) {
				m_editor_state = editor_state::showing_menu;
			}
			else if (m_has_map) {
				m_editor_state    = editor_state::editing_map;
				m_popup_menu_open = false;
			}
		}

		return;
	}

	if (event.type == sf::Event::MouseMoved) {
		if (m_mouse_press_button && *m_mouse_press_button == sf::Mouse::Button::Left) {

			auto in_game_pos = mouse_pos_as_map_pos();
			if (!in_game_pos) {
				return;
			}

			if (!m_last_placed_location || m_last_placed_location->x != in_game_pos->x || m_last_placed_location->y != in_game_pos->y) {
				place_item();
				m_last_placed_location = in_game_pos;
				return;
			}
		}
	}

	if (event.type == sf::Event::MouseButtonPressed) {
		m_mouse_press_location = {event.mouseButton.x, event.mouseButton.y};
		m_mouse_press_button   = event.mouseButton.button;

		if (event.mouseButton.button == sf::Mouse::Button::Left) {
			auto in_game_pos = mouse_pos_as_map_pos();
			if (in_game_pos) {
				m_last_placed_location = in_game_pos;
				place_item();
			}
		}
		return;
	}

	if (event.type == sf::Event::MouseButtonReleased) {
		bool early_exit = false;

		if (!m_mouse_press_location || m_mouse_press_location->x != event.mouseButton.x
		    || m_mouse_press_location->y != event.mouseButton.y) {
			early_exit = true;
		}

		m_mouse_press_location.reset();
		m_mouse_press_button.reset();

		if (early_exit) {
			return;
		}

		if (event.mouseButton.button == sf::Mouse::Button::Right) {
			if (!std::holds_alternative<std::nullopt_t>(m_selection)) {
				m_selection = std::nullopt;
				return;
			}
			m_needs_open_right_click = true;
		}

		return;
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
	sf::Vector2f mpos_f = get_mouse_pos(m_window);
	auto tile_width     = utils::resource_manager::instance().tiles_infos().xspacing;
	auto tile_height    = utils::resource_manager::instance().tiles_infos().yspacing;
	mpos_f.x            = static_cast<float>(static_cast<int>((mpos_f.x) / static_cast<float>(tile_width)) * tile_width);
	mpos_f.y            = static_cast<float>(static_cast<int>((mpos_f.y) / static_cast<float>(tile_height)) * tile_height);

	utils::visitor visitor{
	  [](std::nullopt_t) {},
	  [&](utils::resources_type::mob_id id) {
		  auto sprite = utils::resource_manager::instance().mob_animations(id)->current_sprite();
		  auto height = sprite.getLocalBounds().height;
		  auto width  = sprite.getLocalBounds().width;
		  mpos_f.x += (static_cast<float>(tile_width) - width) / 2;
		  mpos_f.y += (static_cast<float>(tile_height) - height) / 2;

		  sprite.setPosition(mpos_f);
		  m_window.draw(sprite);
	  },
	  [&](utils::resources_type::object_id id) {
		  auto sprite = utils::resource_manager::instance().object_animation(id)->current_sprite();

		  sprite.setPosition(mpos_f);
		  m_window.draw(sprite);
	  },
	  [&](utils::resources_type::tile_id id) {
		  auto sprite = utils::resource_manager::instance().tile_animation(id)->current_sprite();
		  sprite.setPosition(mpos_f);
		  m_window.draw(sprite);
	  },

	};

	std::visit(visitor, m_selection);
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

void view::map_editor::display_field_editor() {
	if (!m_editing_hovered_entity_fields || !m_hovered_entity) {
		return;
	}

	if (!m_field_editor_open) {
		ImGui::OpenPopup(field_editor_menu_name);
		m_field_editor_open = true;
	}

	if (ImGui::BeginPopupModal(field_editor_menu_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		for (std::pair<std::string, adapter::entity_edit_v> &field : m_hovered_entity_fields) {

			auto visitor = [&field](auto &value) {
				using T = std::remove_reference_t<decltype(value)>;

				if constexpr (std::is_integral_v<T>) {
					int val = static_cast<int>(value);
					ImGui::SliderInt(field.first.c_str(), &val, std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
					value = static_cast<T>(val);
				}

				else if constexpr (std::is_same_v<T, float>) {
					ImGui::SliderFloat(field.first.c_str(), &value, std::numeric_limits<float>::min(), std::numeric_limits<float>::max());
				}

				else if constexpr (std::is_same_v<T, std::string>) {
					std::array<char, 128> buf{'\0'};
					std::copy_n(value.data(), std::min(value.size(), buf.size()), buf.data());
					ImGui::InputText(field.first.c_str(), buf.data(), buf.size());
					value = buf.data();
				}

				else if constexpr (std::is_same_v<T, adapter::angle>) {
					ImGui::SliderFloat(field.first.c_str(), &(value.val), -uni::math::pi<float>, uni::math::pi<float>);
				}

				else if constexpr (std::is_same_v<T, adapter::behaviour>) {
					std::string all_behaviours_str; // TODO: add to translation files
					all_behaviours_str += utils::gui_text_for("view.map_editor.bhvr.harmless");
					all_behaviours_str += '\0';
					all_behaviours_str += utils::gui_text_for("view.map_editor.bhvr.patrol");
					all_behaviours_str += '\0';
					all_behaviours_str += utils::gui_text_for("view.map_editor.bhvr.aggressive");
					all_behaviours_str += '\0';
					all_behaviours_str += utils::gui_text_for("view.map_editor.bhvr.dll");
					all_behaviours_str += '\0';
					all_behaviours_str += '\0';

					int val = value.val;
					ImGui::Combo(field.first.c_str(), &val, all_behaviours_str.c_str());
					value.val = static_cast<adapter::behaviour::bhvr>(val);
				}
				else {
					static_assert(false, "unsupported type.");
				}
			};

			std::visit(visitor, field.second);
		}

		if (ImGui::Button(utils::gui_text_for("view.map_editor.editmenu.ok").data())) {
			state::access<map_editor>::adapter(m_state).edit_entity(*m_hovered_entity, m_hovered_entity_fields);
			m_editing_hovered_entity_fields = false;
			m_field_editor_open             = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button(utils::gui_text_for("view.map_editor.editmenu.cancel").data())) {
			m_editing_hovered_entity_fields = false;
			m_field_editor_open             = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::End();
	}
}

void view::map_editor::load_map() {
	state::access<map_editor>::adapter(m_state).load_map(m_file_explorer.selected_path());
	m_has_map = true;
}

void view::map_editor::display_popup() {
	auto &adapter = state::access<map_editor>::adapter(m_state);

	if (m_needs_open_right_click) {
		m_needs_open_right_click = false;
		ImGui::OpenPopup(right_click_menu_name);
	}

	if (ImGui::BeginPopup(right_click_menu_name)) {
		if (!m_hovered_entity) {
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return;
		}

		if (ImGui::Selectable(utils::gui_text_for("view.map_editor.rcmenu.delete").data())) {
			adapter.remove_entity(*m_hovered_entity);
		}

		if (!m_hovered_entity_fields.empty()) {
			if (ImGui::Selectable(utils::gui_text_for("view.map_editor.rcmenu.edit").data())) {
				m_editing_hovered_entity_fields = true;
			}
		}
		if (ImGui::Selectable(utils::gui_text_for("view.map_editor.rcmenu.link_to").data())) {
			// TODO fill stub (for buttons)
		}
		if (ImGui::Selectable(utils::gui_text_for("view.map_editor.rcmenu.toggle").data())) {
			// TODO fill stub (for buttons)
		}
		ImGui::EndPopup();
	}
	else {
		if (!m_editing_hovered_entity_fields) {
			m_hovered_entity = m_map_viewer.hovered_entity();
			if (m_hovered_entity) {
				m_hovered_entity_fields = adapter.entity_properties(*m_hovered_entity);
			}
			else {
				m_hovered_entity_fields.clear();
			}
		}
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
	  std::pair{utils::resources_type::object_id::autoshooter,
	            text_for("view.map_editor.objects.autoshooter")}, // Sprite does not exist yet on ISO (crashes)
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

void view::map_editor::set_map(map_viewer &&viewer) {
	m_map_viewer = std::move(viewer);
	m_map_viewer.set_render_window(m_window);
}

void view::map_editor::place_item() {
	assert(m_mouse_press_location);
	auto pos = mouse_pos_as_map_pos();
	if (!pos) {
		return;
	}

	auto &adapter = state::access<map_editor>::adapter(m_state);

	utils::visitor visitor{
	  [&](std::nullopt_t) {},
	  [&](utils::resources_type::mob_id id) {
		  adapter.add_mob(pos->x, pos->y, id);
	  },
	  [&](utils::resources_type::object_id id) {
		  adapter.add_object(pos->x, pos->y, id);
	  },
	  [&](utils::resources_type::tile_id id) {
		  adapter.edit_tile(pos->x, pos->y, id);
	  },
	};

	std::visit(visitor, m_selection);
}

std::optional<sf::Vector2i> view::map_editor::mouse_pos_as_map_pos() {
	auto tiles_infos = utils::resource_manager::instance().tiles_infos();
	auto level_size  = m_map_viewer.level_size();

	sf::Vector2f mouse_pos = get_mouse_pos(m_window);
	mouse_pos.y /= static_cast<float>(tiles_infos.yspacing);
	mouse_pos.x = (mouse_pos.x - (mouse_pos.y - 1) * static_cast<float>(tiles_infos.y_xshift)) / static_cast<float>(tiles_infos.xspacing);

	if (mouse_pos.x < 0 || mouse_pos.y < 0 || mouse_pos.x >= static_cast<float>(level_size.x)
	    || mouse_pos.y >= static_cast<float>(level_size.y)) {
		return {};
	}

	const sf::Vector2u window_size   = m_window.getSize();
	const sf::Vector2i win_mouse_pos = sf::Mouse::getPosition(m_window);
	if (win_mouse_pos.x <= 0 || win_mouse_pos.y <= 0 || win_mouse_pos.x >= window_size.x || win_mouse_pos.y >= window_size.y) {
		return {};
	}

	return {{static_cast<int>(mouse_pos.x), static_cast<int>(mouse_pos.y)}};
}
