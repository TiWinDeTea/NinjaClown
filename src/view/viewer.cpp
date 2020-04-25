#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <spdlog/spdlog.h>

#include <imgui-SFML.h>
#include <imgui.h>

#include <adapter/adapter.hpp>
#include <imterm/terminal.hpp>

#include "state_holder.hpp"
#include "terminal_commands.hpp"

#include "view/viewer.hpp"

namespace {
namespace cst {
	constexpr unsigned long window_width  = 800;
	constexpr unsigned long window_height = 450;
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

void view::viewer::do_run() noexcept {
	auto &terminal = state::access<view::viewer>::terminal(m_state_holder);

	terminal.set_width(cst::window_width);
	terminal.set_height(cst::window_height / 2);
	terminal.theme() = ImTerm::themes::cherry;
	terminal.log_level(ImTerm::message::severity::info);
	terminal.set_flags(ImGuiWindowFlags_NoTitleBar);
	terminal.disallow_x_resize();
	terminal.filter_hint() = "regex filter...";

	sf::Clock delta_clock;
	sf::Clock music_offset_clock;

	sf::RenderWindow local_window{sf::VideoMode{cst::window_width, cst::window_height}, "Ninja clown !"};
	m_viewport = sf::FloatRect(0.F, 0.F, cst::window_width, cst::window_height);
	sf::Vector2i mouse_pos{0, 0};

	window = &local_window;

	m_window_size = {cst::window_width, cst::window_height};

	bool resized_once = false;
	local_window.setFramerateLimit(std::numeric_limits<unsigned int>::max());
	ImGui::SFML::Init(local_window);

	ImGuiIO &io    = ImGui::GetIO();
	io.IniFilename = nullptr;

	m_fps_limiter.start_now();

	auto step_bot = [this, &terminal]() {
		terminal_commands::argument_type arg{m_state_holder, terminal, {}};
		terminal_commands::update_world(arg);
	};
	bool auto_step_bot   = false;
	bool displaying_term = true;

	while (local_window.isOpen() && m_running) {
		sf::Event event{};
		while (local_window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);

			switch (event.type) {
				case sf::Event::Closed:
					local_window.close();
					break;
				case sf::Event::KeyPressed:
					if (event.key.code == sf::Keyboard::F11) {
						displaying_term = !displaying_term;
					}
					else if (event.key.code == sf::Keyboard::F5) {
						if (!auto_step_bot) {
							step_bot();
						}
					}
					else if (event.key.code == sf::Keyboard::F4) {
						auto_step_bot = !auto_step_bot;
						if (auto_step_bot) {
							terminal_commands::argument_type arg{m_state_holder, terminal, {}};
							terminal_commands::run_model(arg);
						}
						else {
							terminal_commands::argument_type arg{m_state_holder, terminal, {}};
							terminal_commands::stop_model(arg);
						}
					}
					break;
				case sf::Event::Resized: {
					terminal.set_width(local_window.getSize().x);
					if (resized_once) {
						terminal.set_height(std::min(local_window.getSize().y, static_cast<unsigned>(terminal.get_size().y)));
					}
					resized_once = true;

					const float XRATIO = static_cast<float>(event.size.width) / m_window_size.first;
					const float YRATIO = static_cast<float>(event.size.height) / m_window_size.second;

					m_viewport.width *= XRATIO;
					m_viewport.height *= YRATIO;

					m_window_size.first  = event.size.width;
					m_window_size.second = event.size.height;
					window->setView(sf::View{m_viewport});
					break;
				}
				case sf::Event::MouseWheelScrolled:
					if (displaying_term && terminal.get_size().y > event.mouseWheelScroll.y) {
						break;
					}

					if (event.mouseWheelScroll.wheel == sf::Mouse::Wheel::VerticalWheel) {
						const auto MOUSE_POS = to_viewport_coord(sf::Vector2i{event.mouseWheelScroll.x, event.mouseWheelScroll.y});

						const float DELTA = 1.1F;
						float transform;
						if (event.mouseWheelScroll.delta < 0) {
							transform = DELTA;
						}
						else {
							transform = 1 / DELTA;
						}
						sf::FloatRect new_viewport;

						new_viewport.width      = m_viewport.width * transform;
						const float WIDTH_RATIO = new_viewport.width / m_viewport.width;
						new_viewport.left       = m_viewport.left + MOUSE_POS.x * (1 - WIDTH_RATIO);

						new_viewport.height      = m_viewport.height * WIDTH_RATIO;
						const float HEIGHT_RATIO = new_viewport.height / m_viewport.height;
						new_viewport.top         = m_viewport.top + MOUSE_POS.y * (1 - HEIGHT_RATIO);

						m_viewport = new_viewport;
						local_window.setView(sf::View{m_viewport});
					}
					break;
				case sf::Event::MouseMoved:
					if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
						const float zoom_factor = window->getSize().x / m_viewport.width;

						m_viewport.left += (mouse_pos.x - event.mouseMove.x) / zoom_factor;
						m_viewport.top += (mouse_pos.y - event.mouseMove.y) / zoom_factor;
						local_window.setView(sf::View{m_viewport});
					}
					mouse_pos = {event.mouseMove.x, event.mouseMove.y};
					break;
				default:
					break;
			}
		}

		ImGui::SFML::Update(local_window, delta_clock.restart());

		if (displaying_term) {
			ImGui::SetNextWindowPos({0.f, 0.f}, ImGuiCond_Always);
			displaying_term = terminal.show();
			if (close_requested) {
				local_window.close();
			}
		}

		local_window.clear();
		m_map.acquire()->print(*this, m_state_holder.resources);
		if (show_debug_data) {
			m_overmap.acquire()->print_all(*this, state::access<view::viewer>::adapter(m_state_holder), m_state_holder.resources);
		}
		else {
			m_overmap.acquire()->print_all(*this);
		}

		ImGui::SFML::Render(local_window);
		local_window.display();

		m_fps_limiter.wait();
	}
	if (local_window.isOpen()) {
		local_window.close();
	}
	ImGui::SFML::Shutdown();
}

void view::viewer::reload_sprites() {
	m_overmap.acquire()->reload_sprites(m_state_holder.resources);
}

sf::Vector2f view::viewer::to_screen_coords(float x, float y) const noexcept {
	auto [screen_x, screen_y] = to_screen_coords_base(x, y);

	const auto &tiles = m_state_holder.resources.tiles_infos();
	auto max_x        = to_screen_coords_base(static_cast<float>(m_level_size.first + 1), 0).first;
	auto max_y        = to_screen_coords_base(0, static_cast<float>(m_level_size.second + 1)).second;

	float extra_width  = static_cast<float>(m_window_size.first) - max_x;
	float extra_height = static_cast<float>(m_window_size.second) - max_y;

	return {screen_x + extra_width / 2.f, screen_y + extra_height / 2.f};
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
	const auto &tiles = m_state_holder.resources.tiles_infos();
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
