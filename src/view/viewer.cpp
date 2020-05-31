#include <SFML/Graphics/RectangleShape.hpp>
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
#include "view/dialogs.hpp"
#include "view/event_inspector.hpp"
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

void view::viewer::do_run() noexcept {
	auto getterm = [this]() -> ImTerm::terminal<terminal_commands> & {
		return state::access<view::viewer>::terminal(m_state_holder);
	};
	viewer_display_state dp_state{sf::RenderWindow{sf::VideoMode{cst::window_width, cst::window_height}, "Ninja clown !"}, getterm(),
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
	dp_state.viewport = sf::FloatRect(cst::window_width / 4, cst::window_height / 4, 3 * cst::window_width / 4, 3 * cst::window_height / 4);
	dp_state.window.setView(sf::View{dp_state.viewport});

	dp_state.window.setFramerateLimit(std::numeric_limits<unsigned int>::max());
	ImGui::SFML::Init(dp_state.window);

	ImGuiIO &io    = ImGui::GetIO();
	io.IniFilename = nullptr;

	m_fps_limiter.start_now();

	while (dp_state.window.isOpen() && m_running && !close_requested) {

		sf::Event event{};
		while (dp_state.window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);
			inspect_event(*this, event, dp_state);
		}
		m_viewport = dp_state.viewport;

		ImGui::SFML::Update(dp_state.window, dp_state.delta_clock.restart());

		m_dialog_viewer.show(dp_state.window.getSize().x, dp_state.window.getSize().y);

		if (dp_state.displaying_term) {
			ImGui::SetNextWindowPos({0.f, 0.f}, ImGuiCond_Always);
			dp_state.displaying_term = dp_state.terminal.show();
		}

		dp_state.window.clear();
		m_map.acquire()->print(*this, m_state_holder.resources());

		if (show_debug_data && !dp_state.terminal_hovered) {
			m_overmap.acquire()->print_all(*this, state::access<view::viewer>::adapter(m_state_holder), m_state_holder.resources());
		}
		else {
			m_overmap.acquire()->print_all(*this);
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
	auto [screen_x, screen_y] = to_screen_coords_base(x, y);

	const auto &tiles = m_state_holder.resources().tiles_infos();
	auto max_x        = to_screen_coords_base(static_cast<float>(m_level_size.first + 1), 0).first;
	auto max_y        = to_screen_coords_base(0, static_cast<float>(m_level_size.second + 1)).second;

	float extra_width  = static_cast<float>(window->getSize().x) - max_x;
	float extra_height = static_cast<float>(window->getSize().y) - max_y;

	return {screen_x + extra_width / 2.f, screen_y + extra_height / 2.f};
}

void view::viewer::set_map(std::vector<std::vector<map::cell>> &&new_map) noexcept {
	auto map = m_map.acquire();
	map->set(std::move(new_map));
	m_level_size = map->level_size();
}

// conversions necessary to account for the viewport
sf::Vector2f view::viewer::get_mouse_pos() const noexcept {
	const auto &WINDOW_SZ = window->getSize();
	const float YRATIO    = m_viewport.height / WINDOW_SZ.y;
	const float XRATIO    = m_viewport.width / WINDOW_SZ.x;

	const sf::Vector2i MOUSE_POS = sf::Mouse::getPosition(*window);
	return {MOUSE_POS.x * XRATIO + m_viewport.left, MOUSE_POS.y * YRATIO + m_viewport.top};
}

std::pair<float, float> view::viewer::to_screen_coords_base(float x, float y) const noexcept {
	const auto &tiles = m_state_holder.resources().tiles_infos();
	auto xshift       = static_cast<float>(m_level_size.second * tiles.y_xshift);
	auto screen_x     = x * static_cast<float>(tiles.xspacing) + y * static_cast<float>(-tiles.y_xshift) + xshift;
	auto screen_y     = y * static_cast<float>(tiles.yspacing) + x * static_cast<float>(tiles.x_yshift);

	return {screen_x, screen_y};
}

sf::Vector2f view::viewer::to_viewport_coord(const sf::Vector2f &coords) const noexcept {
	const auto WINDOW_SZ      = window->getSize();
	const float VP_WIN_XRATIO = m_viewport.width / WINDOW_SZ.x;
	const float VP_WIN_YRATIO = m_viewport.height / WINDOW_SZ.y;

	return {VP_WIN_XRATIO * coords.x, VP_WIN_YRATIO * coords.y};
}

sf::Vector2f view::viewer::to_viewport_coord(const sf::Vector2i &coords) const noexcept {
	return to_viewport_coord(sf::Vector2f{static_cast<float>(coords.x), static_cast<float>(coords.y)});
}
