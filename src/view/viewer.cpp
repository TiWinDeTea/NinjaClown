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
#include "utils/visitor.hpp"

#include "view/viewer.hpp"

namespace {
namespace cst {
	constexpr unsigned long window_width  = 800;
	constexpr unsigned long window_height = 450;
} // namespace cst
} // namespace

view::viewer::viewer(state::holder *state_holder) noexcept
    : m_state_holder{*state_holder} {}

void view::viewer::run() {
	m_thread  = std::make_unique<std::thread>(&viewer::do_run, this);
	m_running = true;
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

			if (event.type == sf::Event::Closed) {
				local_window.close();
			}
			else if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::F11) {
					displaying_term = !displaying_term;
				}
				else if (event.key.code == sf::Keyboard::F5) {
					step_bot();
				}
				else if (event.key.code == sf::Keyboard::F4) {
					auto_step_bot = !auto_step_bot;
				}
			}
			else if (event.type == sf::Event::Resized) {
				m_window_size.first  = event.size.width;
				m_window_size.second = event.size.height;
				terminal.set_width(local_window.getSize().x);
				if (resized_once) {
					terminal.set_height(std::min(local_window.getSize().y, static_cast<unsigned>(terminal.get_size().y)));
				}
				resized_once = true;
				local_window.setView(
				  sf::View(sf::FloatRect(0.f, 0.f, static_cast<float>(event.size.width), static_cast<float>(event.size.height))));
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

		if (auto_step_bot && !(current_frame() % 15)) {
			step_bot();
		}

		local_window.clear();
		m_map.acquire()->print(*this, m_state_holder.resources);
		{
			auto mobs = m_mobs.acquire();
			for (auto idx = 0u; idx < mobs->size(); ++idx) {
				const auto &mob = (*mobs)[idx];
				mob.print(*this);
				if (show_debug_data && mob.is_hovered(*this)) {
					do_tooltip(local_window, adapter::view_handle{true, idx});
				}
			}
		}
		{
			auto objects = m_objects.acquire();
			for (auto idx = 0u; idx < objects->size(); ++idx) {
				const auto &object = (*objects)[idx];
				object.print(*this);
				if (show_debug_data && object.is_hovered(*this)) {
					do_tooltip(local_window, adapter::view_handle{false, idx});
				}
			}
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

std::pair<float, float> view::viewer::to_screen_coords(float x, float y) const noexcept {
	auto [screen_x, screen_y] = to_screen_coords_base(x, y);

	const auto &tiles = m_state_holder.resources.tiles_infos();
	auto max_x        = to_screen_coords_base(static_cast<float>(m_level_size.first + 1), 0).first;
	auto max_y        = to_screen_coords_base(0, static_cast<float>(m_level_size.second + 1)).second;

	float extra_width  = static_cast<float>(m_window_size.first) - max_x;
	float extra_height = static_cast<float>(m_window_size.second) - max_y;

	return {screen_x + extra_width / 2.f, screen_y + extra_height / 2.f};
}

std::pair<float, float> view::viewer::to_screen_coords_base(float x, float y) const noexcept {
	const auto &tiles = m_state_holder.resources.tiles_infos();
	auto xshift       = static_cast<float>(m_level_size.second * tiles.y_xshift);
	auto screen_x     = x * static_cast<float>(tiles.xspacing) + y * static_cast<float>(-tiles.y_xshift) + xshift;
	auto screen_y     = y * static_cast<float>(tiles.yspacing) + x * static_cast<float>(tiles.x_yshift);

	return {screen_x, screen_y};
}
void view::viewer::do_tooltip(sf::RenderWindow &window, adapter::view_handle handle) noexcept {
	using namespace adapter;
	utils::visitor request_visitor{[&](const request::hitbox &hitbox) {
		                               auto [screen_x, screen_y] = to_screen_coords(hitbox.x, hitbox.y);
		                               auto [screen_width, screen_height]
		                                 = to_screen_coords(hitbox.x + hitbox.width, hitbox.y + hitbox.height);
		                               screen_width -= screen_x;
		                               screen_height -= screen_y;

		                               sf::RectangleShape rect{{screen_width, screen_height}};
		                               rect.setPosition(screen_x, screen_y);
		                               rect.setFillColor(sf::Color{128, 255, 128, 128}); // todo externalize

		                               window.draw(rect);
	                               },
	                               [&](const request::coords &coords) {
		                               m_map.acquire()->highlight_tile(*this, coords.x, coords.y, m_state_holder.resources);
	                               },
	                               [](std::monostate /* ignored */) {}};
	std::visit(request_visitor, state::access<view::viewer>::adapter(m_state_holder).tooltip_for(handle));
}
