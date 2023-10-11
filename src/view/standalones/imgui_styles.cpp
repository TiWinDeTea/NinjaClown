#include <imgui.h>

#include "imgui_styles.hpp"

void view::push_disabled_button_style() noexcept {
	const float grey_val = 0.2f;
	const ImVec4 grey_col = {grey_val, grey_val, grey_val, 1.f};
    ImGui::PushStyleColor(ImGuiCol_Button, grey_col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, grey_col);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, grey_col);
}

void view::pop_disabled_button_style() noexcept {
	ImGui::PopStyleColor(3);
}