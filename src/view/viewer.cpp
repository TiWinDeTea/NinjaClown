#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <spdlog/spdlog.h>

#include <imgui-SFML.h>
#include <imgui.h>
#include <imterm/terminal.hpp>

#include "adapter/adapter.hpp"
#include "state_holder.hpp"
#include "terminal_commands.hpp"
#include "utils/resource_manager.hpp"
#include "view/event_inspector.hpp"
#include "view/file_explorer.hpp"
#include "view/imgui_styles.hpp"
#include "view/viewer.hpp"
#include "view/viewer_display_state.hpp"

namespace {
namespace cst {
	constexpr unsigned long window_width  = 1600;
	constexpr unsigned long window_height = 900;
} // namespace cst
} // namespace

view::viewer::viewer(state::holder *state_holder) noexcept
    : m_state_holder{*state_holder} { }

void view::viewer::run() {
	m_running = true;
	m_thread  = std::make_unique<std::thread>(&viewer::do_run, this);
}

void view::viewer::stop() noexcept {
	m_running = false;
	if (m_thread && m_thread->joinable()) {
		m_thread->join();
	}
}

void view::viewer::wait() {
	if (m_thread && m_thread->joinable()) {
		m_thread->join();
	}
}

#include "view/configurator.hpp"

void view::viewer::do_run() noexcept {

	auto getterm = [this]() -> ImTerm::terminal<terminal_commands> & {
		return state::access<view::viewer>::terminal(m_state_holder);
	};
	viewer_display_state dp_state{configurator{m_state_holder.resources()},
	                              sf::RenderWindow{sf::VideoMode{cst::window_width, cst::window_height}, "Ninja clown !"}, getterm(),
	                              terminal_commands::argument_type{m_state_holder, getterm(), {}}};
	window = &dp_state.window;

	dp_state.terminal.set_width(cst::window_width);
	dp_state.terminal.set_height(cst::window_height / 2);
	dp_state.terminal.theme() = ImTerm::themes::cherry;
	dp_state.terminal.log_level(ImTerm::message::severity::info);
	dp_state.terminal.set_flags(ImGuiWindowFlags_NoTitleBar);
	dp_state.terminal.disallow_x_resize();
	dp_state.terminal.filter_hint() = "regex filter...";

	dp_state.window_size = {cst::window_width, cst::window_height};
	dp_state.viewport    = sf::FloatRect(0, 0, 3 * cst::window_width / 4, 3 * cst::window_height / 4);
	dp_state.window.setView(sf::View{dp_state.viewport});

	dp_state.window.setFramerateLimit(std::numeric_limits<unsigned int>::max());
	ImGui::SFML::Init(dp_state.window);

	ImGuiIO &io    = ImGui::GetIO();
	io.IniFilename = nullptr;

	m_fps_limiter.start_now();

	while (dp_state.window.isOpen() && m_running && !close_requested) {
		sf::Event event{};
		while (dp_state.window.pollEvent(event)) {
			if (inspect_event(*this, event, dp_state)) {
				ImGui::SFML::ProcessEvent(event);
			}
		}

		ImGui::SFML::Update(dp_state.window, dp_state.delta_clock.restart());
		m_viewport = dp_state.viewport;

		if (dp_state.displaying_term) {
			ImGui::SetNextWindowPos({0.f, 0.f}, ImGuiCond_Always);
			dp_state.displaying_term = dp_state.terminal.show();
		}

		dp_state.window.clear();
		m_map.acquire()->print(*this, m_state_holder.resources());

		if (show_debug_data && !dp_state.terminal_hovered && !dp_state.showing_escape_menu) {
			std::vector<std::vector<std::string>> printable_info
			  = m_overmap.acquire()->print_all(*this, state::access<view::viewer>::adapter(m_state_holder), m_state_holder.resources());

			const auto &tiles_infos = m_state_holder.resources().tiles_infos();
			sf::Vector2f mouse_pos  = get_mouse_pos();
			mouse_pos.y /= tiles_infos.yspacing;
			mouse_pos.x = (mouse_pos.x - (mouse_pos.y - 1) * tiles_infos.y_xshift) / tiles_infos.xspacing;

			if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x < m_level_size.first && mouse_pos.y < m_level_size.second) {
				sf::Vector2i win_mouse_pos = sf::Mouse::getPosition(dp_state.window);
				if (win_mouse_pos.x > 0 && win_mouse_pos.y > 0 && win_mouse_pos.x < dp_state.window_size.x
				    && win_mouse_pos.y < dp_state.window_size.y) {
					printable_info.emplace_back().emplace_back("x : " + std::to_string(static_cast<int>(mouse_pos.x)));
					printable_info.back().emplace_back("y : " + std::to_string(static_cast<int>(mouse_pos.y)));

					auto &style = ImGui::GetStyle();
					float x_text_size{0.f};
					float y_text_size{style.WindowPadding.y * 2 - style.ItemInnerSpacing.y};
					for (const std::vector<std::string> &info : printable_info) {
						for (const std::string &str : info) {
							ImVec2 text_size = ImGui::CalcTextSize(str.data(), str.data() + str.size());
							y_text_size += text_size.y;
							y_text_size += style.ItemInnerSpacing.y;
							x_text_size = std::max(x_text_size, text_size.x);
						}
						y_text_size += style.ItemInnerSpacing.y;
					}
					x_text_size += style.WindowPadding.x * 2;

					float prev = std::exchange(style.WindowRounding, 0.f);
					ImVec2 pos = {0, dp_state.window_size.y - y_text_size};
					ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
					ImGui::SetNextWindowSize(ImVec2{x_text_size, y_text_size});
					if (ImGui::Begin("##corner info window", nullptr,
					                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar
					                   | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse
					                   | ImGuiWindowFlags_NoScrollbar)) {
						for (auto it = printable_info.cbegin(), end = std::prev(printable_info.cend()); it != end; ++it) {
							for (const std::string &str : *it) {
								ImGui::Text("%s", str.c_str());
							}
							ImGui::Separator();
						}
						for (const std::string &str : printable_info.back()) {
							ImGui::Text("%s", str.c_str());
						}
					}
					ImGui::End();
					style.WindowRounding = prev;
				}
			}
		}
		else {
			m_overmap.acquire()->print_all(*this);
		}

		show_menu_window(dp_state);
		dp_state.configurator.give_control();
		if (dp_state.configurator.were_graphics_changed()) {
			m_overmap.acquire()->reload_sprites(m_state_holder.resources());
		}

		ImGui::SFML::Render(dp_state.window);
		dp_state.window.display();

		m_fps_limiter.wait();
	}

	if (dp_state.window.isOpen()) {
		dp_state.window.close();
	}
	ImGui::SFML::Shutdown();
}

void view::viewer::show_menu_window(viewer_display_state &state) noexcept {
	constexpr const char *menu_window_name = "##in game menu popup";

	if (!state.showing_escape_menu) {
		if (state.escape_menu_currently_open) {
			state.escape_menu_currently_open = false;
			if (ImGui::BeginPopupModal(menu_window_name, nullptr, ImGuiWindowFlags_NoTitleBar)) {
				state.explorer.close();
				state.explorer.give_control(m_state_holder.resources());
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
			}
		}
		return;
	}

	if (!state.escape_menu_currently_open) {
		ImGui::OpenPopup(menu_window_name);
		state.escape_menu_currently_open = true;
	}

	const auto &res   = m_state_holder.resources();
	const auto &style = ImGui::GetStyle();

	std::string_view resume   = res.gui_text_for("view.in_game_menu.resume");
	std::string_view load_dll = res.gui_text_for("view.in_game_menu.dll");
	std::string_view restart  = res.gui_text_for("view.in_game_menu.restart");
	std::string_view settings = res.gui_text_for("view.in_game_menu.settings");
	std::string_view load_map = res.gui_text_for("view.in_game_menu.load_map");
	std::string_view import   = res.gui_text_for("view.in_game_menu.import_maps");
	std::string_view quit     = res.gui_text_for("view.in_game_menu.quit");

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
	update_sz(quit);

	float text_width = max_text_size.x + style.ItemInnerSpacing.x * 2;
	ImGui::SetNextWindowSize(ImVec2{text_width + style.WindowPadding.x * 2, 0.f});
	if (ImGui::BeginPopupModal(menu_window_name, nullptr,
	                           ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {

		if (!state::access<viewer>::adapter(m_state_holder).map_is_loaded()) {
			using_style(disabled_button) {
                ImGui::Button(resume.data(), ImVec2{text_width, 0.f});
                ImGui::Button(restart.data(), ImVec2{text_width, 0.f});
			};
		} else {
            if (ImGui::Button(resume.data(), ImVec2{text_width, 0.f})) {
                state.showing_escape_menu = false;
				if (state.autostep_bot) {
					terminal_commands::run_model(state.empty_arg);
				}
			}
			if (ImGui::Button(restart.data(), ImVec2{text_width, 0.f})) {
				state::access<view::viewer>::adapter(m_state_holder).load_map(m_state_holder.current_map_path());
				state.autostep_bot        = false;
				state.showing_escape_menu = false;
            }
        }

        if (ImGui::Button(load_dll.data(), ImVec2{text_width, 0.f})) {
            state.explorer.open(with_extensions{".dll", ".so"});
        }

        using_style(disabled_button) {
			if (ImGui::Button(load_map.data(), ImVec2{text_width, 0.f})) {
			}
			if (ImGui::Button(import.data(), ImVec2{text_width, 0.f})) {
			}
		};

        if (ImGui::Button(settings.data(), ImVec2{text_width, 0.f})) {
            state.configurator.show();
        }

        if (ImGui::Button(quit.data(), ImVec2{text_width, 0.f})) {
			state.window.close();
		}

		state.explorer.give_control(m_state_holder.resources());
		if (state.explorer.path_ready()) {
			auto arg_copy = state.empty_arg;
			arg_copy.command_line.emplace_back("");
			arg_copy.command_line.emplace_back(state.explorer.selected_path().generic_string());
			terminal_commands::load_shared_library(arg_copy);
			state.showing_escape_menu = false;
			state.autostep_bot = false;
		}
		ImGui::EndPopup();
	}
}

void view::viewer::reload_sprites() {
	m_overmap.acquire()->reload_sprites(m_state_holder.resources());
}

utils::synchronized<view::overmap_collection>::acquired_t view::viewer::acquire_overmap() noexcept {
	return m_overmap.acquire();
}

utils::synchronized<view::map, utils::spinlock>::acquired_t view::viewer::acquire_map() noexcept {
	return m_map.acquire();
}

sf::Vector2f view::viewer::to_screen_coords(float x, float y) const noexcept {
	const auto &tiles = m_state_holder.resources().tiles_infos();
	sf::Vector2f screen;

	screen.x = x * tiles.xspacing + y * tiles.y_xshift;
	screen.y = x * tiles.x_yshift + y * tiles.yspacing;

	return screen;
}

void view::viewer::set_map(std::vector<std::vector<map::cell>> &&new_map) noexcept {
	auto map = m_map.acquire();
	map->set(std::move(new_map));
	m_level_size = map->level_size();
}

// conversions necessary to account for the viewport
sf::Vector2f view::viewer::get_mouse_pos() const noexcept {
	assert(window);
	return to_viewport_coord(sf::Mouse::getPosition(*window));
}

sf::Vector2f view::viewer::to_viewport_coord(const sf::Vector2f &coords) const noexcept {
	const auto WINDOW_SZ      = window->getSize();
	const float VP_WIN_XRATIO = m_viewport.width / WINDOW_SZ.x;
	const float VP_WIN_YRATIO = m_viewport.height / WINDOW_SZ.y;

	return {VP_WIN_XRATIO * coords.x + m_viewport.left, VP_WIN_YRATIO * coords.y + m_viewport.top};
}

sf::Vector2f view::viewer::to_viewport_coord(const sf::Vector2i &coords) const noexcept {
	return to_viewport_coord(sf::Vector2f{static_cast<float>(coords.x), static_cast<float>(coords.y)});
}
