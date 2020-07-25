#include <imgui.h>

#include "view/imgui_styles.hpp"

void view::push_disabled_button_style() noexcept {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2, 0.2, 0.2, 1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2, 0.2, 0.2, 1});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2, 0.2, 0.2, 1});
}

void view::pop_disabled_button_style() noexcept {
	ImGui::PopStyleColor(3);
}