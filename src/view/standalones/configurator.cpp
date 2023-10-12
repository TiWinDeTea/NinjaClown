#include <imgui.h>

#include "configurator.hpp"
#include "imgui_styles.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"

// TODO : rendre la classe configurator statique (pas vraiment besoin de l’instancier plusieurs fois)

using lang_info = std::remove_const_t<std::remove_reference_t<
  std::invoke_result<decltype(&utils::resource_manager::get_language_list), utils::resource_manager>::type>>::value_type;

namespace {

namespace idx {
	enum idx {
		general_lang,
		command_lang,
		gui_lang,
		log_lang,
		resource_pack,
		MAX,
	};
}

constexpr const char *window_name = "##configurator";

std::string display_name(const utils::resource_manager::resource_pack_info &resource_pack, const lang_info &user_lang) {
	if (auto it = resource_pack.names_by_shorthand_lang.find(user_lang.map_name); it != resource_pack.names_by_shorthand_lang.end()) {
		return it->second;
	}
	return resource_pack.default_name;
}

std::string display_name(const lang_info &lang) {
	std::string ans = lang.name;
	if (!lang.shorthand.empty()) {
		ans += " (" + lang.variant + ")";
	}
	return ans;
}

template <typename T>
const T *combo(std::string_view label, const std::vector<T> &choices, const lang_info &current_lang, float label_width, float combo_width) {
	// int idx = static_cast<int>(std::distance(langs.begin(), std::find(langs.begin(), langs.end(), current_lang)));

	const T *selected = nullptr;

	ImGui::TextUnformatted(label.data(), label.data() + label.size());
	ImGui::SameLine(label_width);

	ImGui::PushID(label.data(), label.data() + label.size());
	ImGui::SetNextItemWidth(combo_width);
	if (ImGui::BeginCombo("##lang_combo", display_name(current_lang).c_str(), ImGuiComboFlags_NoArrowButton)) {
		for (const T &current : choices) {
			bool is_selected = (current == current_lang);
			if (ImGui::Selectable(display_name(current).c_str(), is_selected)) {
				selected = &current;
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopID();

	return selected;
}
} // namespace

void view::configurator::give_control() noexcept {
	auto &resources = utils::resource_manager::instance();

	m_graphics_changed = false;
	if (!m_showing) {
		if (m_popup_open) {
			if (ImGui::BeginPopupModal(window_name, nullptr, ImGuiWindowFlags_NoTitleBar)) {
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				m_popup_open = false;
			}

			if (m_config_must_be_saved) {
				if (!resources.save_user_config()) {
					utils::log::warn("view.configurator.config_save_failed");
				}
				m_config_must_be_saved = false;
			}
		}
		return;
	}
	if (!m_popup_open) {
		ImGui::OpenPopup(window_name);
		m_popup_open = true;
		resources.refresh_language_list();
		resources.refresh_resource_pack_list();
	}

	if (!ImGui::BeginPopupModal(window_name, nullptr,
	                            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize // NOLINT(*-signed-bitwise)
	                              | ImGuiWindowFlags_AlwaysAutoResize)) {
		m_popup_open = false;
		m_showing    = false;
		return;
	}
	const ImGuiStyle &style = ImGui::GetStyle();

	std::array<std::string_view, idx::MAX> labels;
	labels[idx::general_lang]  = resources.gui_text_for("configurator.general_lang");
	labels[idx::command_lang]  = resources.gui_text_for("configurator.command_lang");
	labels[idx::gui_lang]      = resources.gui_text_for("configurator.gui_lang");
	labels[idx::log_lang]      = resources.gui_text_for("configurator.log_lang");
	labels[idx::resource_pack] = resources.gui_text_for("configurator.resource_pack");

	float labels_width{0};
	for (const std::string_view str : labels) {
		labels_width = std::max(ImGui::CalcTextSize(str.data(), str.data() + str.size()).x, labels_width);
	}
	labels_width += style.ItemSpacing.x;

	// langs
	{

		const auto &langs = resources.get_language_list();
		float combo_width{0};
		for (const auto &lang : langs) {
			combo_width = std::max(ImGui::CalcTextSize(display_name(lang).c_str()).x, combo_width);
		}
		combo_width += style.ItemInnerSpacing.x;

		const lang_info *selection = nullptr;
		if (selection = combo(labels[idx::general_lang], langs, resources.user_general_lang(), labels_width, combo_width);
		    selection != nullptr) {
			resources.set_user_general_lang(*selection);
			m_config_must_be_saved = true;
		}
		if (selection = combo(labels[idx::command_lang], langs, resources.user_commands_lang(), labels_width, combo_width);
		    selection != nullptr) {
			resources.set_user_command_lang(*selection);
			m_config_must_be_saved = true;
		}
		if (selection = combo(labels[idx::gui_lang], langs, resources.user_gui_lang(), labels_width, combo_width); selection != nullptr) {
			resources.set_user_gui_lang(*selection);
			m_config_must_be_saved = true;
		}
		if (selection = combo(labels[idx::log_lang], langs, resources.user_log_lang(), labels_width, combo_width); selection != nullptr) {
			resources.set_user_log_lang(*selection);
			m_config_must_be_saved = true;
		}

		std::string_view import_lang = resources.gui_text_for("configurator.import_lang");
		using_style(disabled_button) {
			if (ImGui::Button(import_lang.data(), {ImGui::GetContentRegionAvail().x, 0})) {
			}
		};
	}

	ImGui::Separator();

	// resource pack
	{
		const std::vector<utils::resource_manager::resource_pack_info> &resource_packs = resources.get_resource_pack_list();

		ImGui::TextUnformatted(labels[idx::resource_pack].data(), labels[idx::resource_pack].data() + labels[idx::resource_pack].size());
		ImGui::SameLine(labels_width);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##configurator.resource_pack",
		                      display_name(resources.user_resource_pack(), resources.user_gui_lang()).c_str(),
		                      ImGuiComboFlags_NoArrowButton)) {
			for (const auto &res_pack : resource_packs) {
				bool is_selected = (res_pack.file == resources.user_gui_lang().file);
				if (ImGui::Selectable(display_name(res_pack, resources.user_gui_lang()).c_str(), is_selected)) {
					resources.set_user_resource_pack(res_pack);
					m_graphics_changed = true;
				}
				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		std::string_view import_respack = resources.gui_text_for("configurator.import_respack");
		using_style(disabled_button) {
			if (ImGui::Button(import_respack.data(), {ImGui::GetContentRegionAvail().x, 0})) {
			}
		};
	}

	ImGui::EndPopup();

	// TODO: bouton pour quitter le menu de configuration
}
