#include <imgui.h>

#include "utils/resource_manager.hpp"
#include "utils/system.hpp"
#include "view/file_explorer.hpp"


// TODO : rendre la classe file_explorer statique (pas vraiment besoin de l’instancier plusieurs fois)

// TODO : boutons précédent/suivant

// TODO : sauvegarder le dernier dossier accédé dans la config ?

namespace {
constexpr const char window_name[] = "##file explorer";
std::optional<std::filesystem::path> glob_last_explored_folder{}; // TODO : si la classe est statique, cette variable est superflue (par rapport aux variables déjà présentes dans la classe)
}

void view::file_explorer::give_control(const utils::resource_manager &res) noexcept {
	if (m_showing) {
		if (!m_was_showing) {
			ImGui::OpenPopup(window_name);
		}
		m_was_showing = true;

		ImGui::SetNextWindowSize(ImVec2{800, 400}, ImGuiCond_FirstUseEver);
		if (ImGui::BeginPopupModal(window_name)) {
			auto &style = ImGui::GetStyle();

			float text_height = ImGui::CalcTextSize(m_current.generic_string().c_str()).y;
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0x0B / 255.f, 0x36 / 255.f, 0x85 / 255.f, 1.f});
			if (ImGui::BeginChild("##full_path", ImVec2{0, text_height + style.ItemInnerSpacing.y * 2}), true) {
				std::vector<std::string> splited_path;

				bool is_file = std::filesystem::is_regular_file(m_current);
				for (std::filesystem::path path = is_file ? m_current.parent_path() : m_current; path.parent_path() != path;
				     path                       = path.parent_path()) {

					splited_path.emplace_back(path.filename().generic_string());
				}
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ChildBg));

				bool back_to_root = ImGui::Button("@");
				ImGui::SameLine(0.f, 0.f);
				ImGui::TextUnformatted("/");

				unsigned int select = 0;
				if (splited_path.size() > 1) {
					for (unsigned int i = splited_path.size() - 1; i != 0; --i) {
						ImGui::SameLine(0.f, 0.f);
						if (ImGui::Button(splited_path[i].c_str())) {
							select = i;
						}
						ImGui::SameLine(0.f, 0.f);
						ImGui::TextUnformatted("/");
					}
				}

				if (!splited_path.empty()) {
					ImGui::SameLine(0.f, 0.f);
					if (ImGui::Button(splited_path.front().c_str())) {
						select = 0;
					}

					while (select > 0) {
						m_current = m_current.parent_path();
						--select;
					}
				}

				ImGui::PopStyleColor();

				if (back_to_root) {
					m_current = "/";
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleColor();

			auto window_bg = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
			window_bg.w    = 1.f;
			ImGui::PushStyleColor(ImGuiCol_ChildBg, window_bg);
			if (ImGui::BeginChild("##file displayer", ImVec2{0, ImGui::GetContentRegionAvail().y - style.ItemSpacing.y * 2
			                                                      - style.ItemInnerSpacing.y * 2 - text_height})) {

				std::filesystem::path working_directory;
				if (std::filesystem::is_regular_file(m_current)) {
					working_directory = m_current.parent_path();
				}
				else {
					working_directory = m_current;
				}

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0xCB / 255.f, 0x33 / 255.f, 0x25 / 255.f, 1.f});

                if (ImGui::Selectable("../", working_directory.parent_path() == m_currently_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    m_currently_selected = working_directory.parent_path();
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        m_current = m_currently_selected;
                    }
                }

				for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator{working_directory}) {
					if (!file.is_directory()) {
						continue;
					}
					std::string file_name = file.path().filename().generic_string() + "/";

					if (ImGui::Selectable(file_name.c_str(), file.path() == m_currently_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
						if (ImGui::IsMouseDoubleClicked(0)) {
							m_current = file.path();
						}
						m_currently_selected = file.path();
					}
				}
				ImGui::PopStyleColor();

				for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator{working_directory}) {
					if (file.is_directory()) {
						continue;
					}

					std::string file_name = file.path().filename().generic_string();

					unsigned int pop = 0;
					if (std::find(m_prefered_extensions.begin(), m_prefered_extensions.end(), file.path().extension().generic_string()) != m_prefered_extensions.end()) {
						pop++;
						ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0xD9 / 255.f, 0xD9 / 255.f, 0x26 / 255.f, 1.f});
					}

					if (ImGui::Selectable(file_name.c_str(), file.path() == m_currently_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
						if (ImGui::IsMouseDoubleClicked(0)) {
							m_current = file.path();
							if (file.is_regular_file()) {
								m_current     = file.path();
								m_path_ready  = true;
								m_showing     = false;
								m_was_showing = false;
								ImGui::CloseCurrentPopup();
							}
						}
						m_currently_selected = file.path();
					}

					ImGui::PopStyleColor(pop);
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleColor();

			std::string_view ok_text     = res.gui_text_for("file_explorer.ok_button");
			std::string_view cancel_text = res.gui_text_for("file_explorer.cancel_button");

			float text_width = ImGui::CalcTextSize(ok_text.data(), ok_text.data() + ok_text.size()).x
			                   + ImGui::CalcTextSize(cancel_text.data(), cancel_text.data() + cancel_text.size()).x;

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - text_width - style.ItemSpacing.x * 4
			                     - style.ItemInnerSpacing.x * 4);

            glob_last_explored_folder = m_current;

			if (ImGui::Button(std::string{ok_text}.c_str())) {
				m_current     = m_currently_selected;
				m_path_ready  = true;
				m_showing     = false;
				m_was_showing = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button(std::string{cancel_text}.c_str())) {
				m_showing = false;
				m_was_showing = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
	else if (m_was_showing) {
		m_was_showing = false;
		if (ImGui::BeginPopupModal(window_name)) {
			ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
	}
}

void view::file_explorer::open(with_extensions exts) noexcept {
	m_currently_selected.clear();
	m_showing = true;
	m_prefered_extensions = std::move(exts.exts);

	if (glob_last_explored_folder) {
		m_current = *glob_last_explored_folder;
	} else {
        m_current = utils::binary_directory().parent_path();
	}
}

void view::file_explorer::open(std::filesystem::path path, with_extensions exts) noexcept {
	m_currently_selected.clear();
	m_current = std::move(path);
	m_showing = true;
    m_prefered_extensions = std::move(exts.exts);
}
