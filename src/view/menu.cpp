#include "menu.hpp"
#include "adapter/adapter.hpp"
#include "state_holder.hpp"
#include "utils/resource_manager.hpp"
#include "view/imgui_styles.hpp"

#include <imgui.h>

namespace {
constexpr const char *menu_window_name = "##in game menu popup";
}

view::menu::menu(::state::holder &state) noexcept
    : m_state{state}
    , m_configurator{state.resources()} { }

void view::menu::close() {
	if (m_currently_open) {
		m_currently_open = false;
		if (ImGui::BeginPopupModal(menu_window_name, nullptr, ImGuiWindowFlags_NoTitleBar)) {
			m_explorer.close();
			m_explorer.give_control(m_state.resources());
			m_configurator.close();
			m_configurator.give_control();
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
	}
}

view::menu::user_request view::menu::show() {
	if (!m_currently_open) {
		ImGui::OpenPopup(menu_window_name);
		m_currently_open = true;
	}

	const auto &res   = m_state.resources();
	const auto &style = ImGui::GetStyle();

	const std::string_view resume          = res.gui_text_for("view.in_game_menu.resume");
	const std::string_view load_dll        = res.gui_text_for("view.in_game_menu.dll");
	const std::string_view restart         = res.gui_text_for("view.in_game_menu.restart");
	const std::string_view settings        = res.gui_text_for("view.in_game_menu.settings");
	const std::string_view load_map        = res.gui_text_for("view.in_game_menu.load_map");
	const std::string_view import          = res.gui_text_for("view.in_game_menu.import_maps");
	const std::string_view credits         = res.gui_text_for("view.in_game_menu.credits");
	const std::string_view map_editor      = res.gui_text_for("view.in_game_menu.map_editor");
	const std::string_view campaign_editor = res.gui_text_for("view.in_game_menu.campaign_editor");
	const std::string_view quit            = res.gui_text_for("view.in_game_menu.quit");

	ImVec2 max_text_size{0.f, 0.f};
	auto update_sz = [&max_text_size](std::string_view str) {
		auto size       = ImGui::CalcTextSize(str.data(), str.data() + str.size());
		max_text_size.x = std::max(max_text_size.x, size.x);
		max_text_size.y = std::max(max_text_size.y, size.y);
	};
	update_sz(resume);
	update_sz(load_dll);
	update_sz(restart);
	update_sz(settings);
	update_sz(load_map);
	update_sz(import);
	update_sz(credits);
	update_sz(map_editor);
	update_sz(campaign_editor);
	update_sz(quit);

	float text_width = max_text_size.x + style.ItemInnerSpacing.x * 2;
	ImGui::SetNextWindowSize(ImVec2{text_width + style.WindowPadding.x * 2, 0.f});
	if (ImGui::BeginPopupModal(menu_window_name, nullptr,
	                           ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) { // NOLINT(hicpp-signed-bitwise)

		if (!::state::access<menu>::adapter(m_state).map_is_loaded()) {
			using_style(disabled_button) {
				ImGui::Button(resume.data(), ImVec2{text_width, 0.f});
				ImGui::Button(restart.data(), ImVec2{text_width, 0.f});
			};
		}
		else {
			if (ImGui::Button(resume.data(), ImVec2{text_width, 0.f})) {
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return user_request::close_menu;
			}
			if (ImGui::Button(restart.data(), ImVec2{text_width, 0.f})) {
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return user_request::restart;
			}
		}

		if (ImGui::Button(load_dll.data(), ImVec2{text_width, 0.f})) {
			m_current_state    = state::filesystem;
			m_current_substate = substate::loading_dll;
			m_explorer.open(with_extensions{".dll", ".so"});
		}

		if (ImGui::Button(load_map.data(), ImVec2{text_width, 0.f})) {
			m_current_state    = state::filesystem;
			m_current_substate = substate::loading_map;
			m_explorer.open(with_extensions{".map"});
		}

		using_style(disabled_button) {
			ImGui::Button(import.data(), ImVec2{text_width, 0.f}); // TODO
		};

		if (ImGui::Button(settings.data(), ImVec2{text_width, 0.f})) {
			m_current_state    = state::config;
			m_current_substate = substate::na;
			m_configurator.show();
		}

		using_style(disabled_button) {
			ImGui::Button(credits.data(), ImVec2{text_width, 0.f}); // TODO
			ImGui::Button(map_editor.data(), ImVec2{text_width, 0.f}); // TODO
			ImGui::Button(campaign_editor.data(), ImVec2{text_width, 0.f}); // TODO
		};

		if (ImGui::Button(quit.data(), ImVec2{text_width, 0.f})) {
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
			return user_request::close_window;
		}

		switch (m_current_state) {
			case state::none:
				break;
			case state::config:
				m_configurator.give_control();
				break;
			case state::filesystem:
				m_explorer.give_control(m_state.resources());
				if (m_explorer.path_ready()) {
					m_path = m_explorer.selected_path();
					m_explorer.close();
					m_explorer.give_control(m_state.resources());

					switch (m_current_substate) {
						case substate::na:
							// todo : log error
							break;
						case substate::loading_dll:
							ImGui::EndPopup();
							return user_request::load_dll;
						case substate::loading_map:
							ImGui::EndPopup();
							return user_request::load_map;
					}
					m_current_state = state::none;
				}
		}

		ImGui::EndPopup();
	}
	return user_request::none;
}