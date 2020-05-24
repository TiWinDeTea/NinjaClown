#include <imgui.h>
#include <spdlog/spdlog.h>

#include "utils/scope_guards.hpp"
#include "view/dialogs.hpp"

// TODO externalize
namespace {
inline const float DIALOG_X_MARGIN = 5;
inline const float DIALOG_Y_MARGIN = 5;
inline const float DIALOG_HEIGHT   = 50;
inline const float DIALOG_AUTHOR_MARGIN = 15;

void draw_window(const char *window_name, view::word text) {
    constexpr unsigned int window_flags
      = static_cast<unsigned int>(ImGuiWindowFlags_NoTitleBar) | static_cast<unsigned int>(ImGuiWindowFlags_NoResize)
        | static_cast<unsigned int>(ImGuiWindowFlags_NoMove) | static_cast<unsigned int>(ImGuiWindowFlags_NoScrollbar);

	const bool opened = ImGui::Begin(window_name, nullptr, window_flags);
	ON_SCOPE_EXIT {
		ImGui::End();
	};

	if (opened) {
		ImGui::Text("%s", text.author.c_str());
		ImGui::Dummy({DIALOG_AUTHOR_MARGIN, 1});
		ImGui::SameLine();
		ImGui::Text("%s", text.sentence.c_str());
	}
	else {
		spdlog::warn("Failed to open dialog window {}", window_name);
	}
}
} // namespace

void view::dialog_viewer::select_dialog(std::vector<dialog>::size_type dialog_idx) noexcept {
	m_current_dialog = dialog_idx;
}

void view::dialog_viewer::clear() {
	m_dll_word_dirty = false;
	m_dll_word = {};
	m_dialogs.clear();
	m_current_dialog.reset();
}

void view::dialog_viewer::dll_word(std::string word, std::chrono::milliseconds duration) noexcept {
	auto dll_word_data    = m_dll_word_synced.acquire();
	dll_word_data->first  = std::move(word);
	dll_word_data->second = std::chrono::system_clock::now() + duration;
	m_dll_word_dirty      = true;
}

bool view::dialog_viewer::on_click([[maybe_unused]] int x, [[maybe_unused]] int y) noexcept {
	if (m_current_dialog) {
		// TODO : test x & y against popup pos
		dialog &current = m_dialogs[*m_current_dialog];
		if (current.current_word < current.words.size()) {
			++current.current_word;
		}
		else {
			m_current_dialog.reset();
		}
		return true;
	}
	return false;
}

void view::dialog_viewer::show(unsigned int win_length, unsigned int win_height) noexcept {

	if (m_dll_word_dirty) {
		auto new_dll_word = m_dll_word_synced.acquire();
		m_dll_word        = std::move(*new_dll_word);
		m_dll_word_dirty  = false;
	}
	if (!m_dll_word.first.empty()) {
		if (std::chrono::system_clock::now() < m_dll_word.second) {
			ImGui::SetNextWindowPos({DIALOG_X_MARGIN, DIALOG_Y_MARGIN}, ImGuiCond_Always);
			ImGui::SetNextWindowSize({static_cast<float>(win_length) - 2 * DIALOG_X_MARGIN, DIALOG_HEIGHT}, ImGuiCond_Always);
			draw_window("dll dialog", {"You", m_dll_word.first}); // TODO externalize
		}
		else {
			m_dll_word.first.clear();
		}
	}

	if (m_current_dialog) {
        dialog& dlg = m_dialogs[*m_current_dialog];
		if (dlg.current_word < dlg.words.size()) {
			ImGui::SetNextWindowPos({DIALOG_X_MARGIN, static_cast<float>(win_height) - DIALOG_HEIGHT - DIALOG_Y_MARGIN}, ImGuiCond_Always);
			ImGui::SetNextWindowSize({static_cast<float>(win_length) - 2 * DIALOG_X_MARGIN, DIALOG_HEIGHT}, ImGuiCond_Always);
			draw_window("map dialog", dlg.words[dlg.current_word]);
		}
	}
}