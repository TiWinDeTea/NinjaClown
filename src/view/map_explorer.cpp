#include "view/map_explorer.hpp"

#include <imgui.h>
#include <utility>

namespace {
constexpr const char window_name[] = "##map explorer";
}

void view::map_explorer::give_control(const utils::resource_manager &resources) noexcept {
	if (!m_showing) {
		if (m_was_showing) {
			m_was_showing = false;
			if (ImGui::BeginPopupModal(window_name)) {
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}
		}
		return;
	} // else


	if (!m_was_showing) {
		ImGui::OpenPopup(window_name);
        m_was_showing = true;
    }

	ImGui::SetNextWindowSize(ImVec2{800, 400}, ImGuiCond_FirstUseEver);
	if (ImGui::BeginPopupModal(window_name)) {
		auto &style = ImGui::GetStyle();

	}
}
