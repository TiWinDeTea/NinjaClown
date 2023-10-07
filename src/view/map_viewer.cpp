#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>

#include <imgui.h>
#include <utility>

#include "state_holder.hpp"
#include "utils/resource_manager.hpp"
#include "view/map_viewer.hpp"

view::map_viewer::map_viewer(state::holder &state) noexcept
    : m_state{&state} { }

void view::map_viewer::print(bool show_debug_data) {
	assert(m_window);
	assert(m_state);

	++m_current_frame;
	m_map.acquire()->print(*this, m_state->resources());

	if (!show_debug_data) {
		m_overmap.acquire()->print_all(*this);
		return;
	}

	sf::Vector2u window_size = m_window->getSize();

	std::vector<std::vector<std::string>> printable_info
	  = m_overmap.acquire()->print_all(*this, state::access<map_viewer>::adapter(*m_state), m_state->resources());

	const auto &tiles_infos = m_state->resources().tiles_infos();
	sf::Vector2f mouse_pos  = get_mouse_pos();
	mouse_pos.y /= tiles_infos.yspacing;
	mouse_pos.x = (mouse_pos.x - (mouse_pos.y - 1) * tiles_infos.y_xshift) / tiles_infos.xspacing;

	if (mouse_pos.x >= 0 && mouse_pos.y >= 0 && mouse_pos.x < m_level_size.x && mouse_pos.y < m_level_size.y) {
		sf::Vector2i win_mouse_pos = sf::Mouse::getPosition(*m_window);
		if (win_mouse_pos.x > 0 && win_mouse_pos.y > 0 && win_mouse_pos.x < window_size.x && win_mouse_pos.y < window_size.y) {
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
			ImVec2 pos = {0, window_size.y - y_text_size};
			ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2{x_text_size, y_text_size});
			if (ImGui::Begin("##corner info window", nullptr,
			                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar
			                   | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar)) {
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

void view::map_viewer::draw(sf::Sprite &d) {
	assert(m_window);
	m_window->draw(d);
}

void view::map_viewer::draw(sf::RectangleShape &d) {
	assert(m_window);
	m_window->draw(d);
}

void view::map_viewer::highlight_tile(sf::Vector2i tile_coord) {
	m_map.acquire()->highlight_tile(*this, tile_coord.x, tile_coord.y, m_state->resources());
}

void view::map_viewer::reload_sprites() {
	m_overmap.acquire()->reload_sprites(m_state->resources());
}

// conversions necessary to account for the viewport
sf::Vector2f view::map_viewer::get_mouse_pos() const noexcept {
	auto view = m_window->getView();
	auto size = m_window->getSize();
	sf::Vector2f ratio{view.getSize().x / size.x, view.getSize().y / size.y};

	sf::Vector2f mouse(sf::Mouse::getPosition(*m_window));
	mouse.x *= ratio.x;
	mouse.y *= ratio.y;

	return mouse + view.getCenter() - view.getSize() / 2.f;
}

sf::Vector2f view::map_viewer::to_screen_coords(float x, float y) const noexcept {
	const auto &tiles = m_state->resources().tiles_infos();
	sf::Vector2f screen;

	screen.x = x * tiles.xspacing + y * tiles.y_xshift;
	screen.y = x * tiles.x_yshift + y * tiles.yspacing;

	return screen;
}
