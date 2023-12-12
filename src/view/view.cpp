#include "view/view.hpp"
#include "adapter/adapter.hpp"
#include "adapter/adapter_classes.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"
#include "terminal_commands.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"
#include "utils/system.hpp"
#include "view/game/game_viewer.hpp"
#include "view/map_editor/map_editor.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <SFML/Window/Event.hpp>
#include <imgui-SFML.h>
#include <imterm/terminal.hpp>
#include <memory>

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <spdlog/spdlog.h>
#include <thread>

using fmt::operator""_a;

namespace {
bool is_mouse_event(sf::Event::EventType type) {
	return type == sf::Event::MouseWheelMoved || type == sf::Event::MouseWheelScrolled || type == sf::Event::MouseButtonPressed
	       || type == sf::Event::MouseButtonReleased || type == sf::Event::MouseMoved || type == sf::Event::MouseEntered
	       || type == sf::Event::MouseLeft;
}
bool is_kb_event(sf::Event::EventType type) {
	return type == sf::Event::TextEntered || type == sf::Event::KeyPressed || type == sf::Event::KeyReleased;
}
bool is_fallthrough_key(sf::Keyboard::Key key) {
	using K = sf::Keyboard::Key;
	return (key >= K::F1 && key <= K::F12) || key == K::Escape;
}


constexpr const auto fontawesome_path = "fonts/fontawesome-free-5.4.0-desktop/" FONT_ICON_FILE_NAME_FAS;

void setup_terminal(ImTerm::terminal<terminal_commands> &terminal, unsigned int x_size, unsigned int y_size) {
	terminal.set_width(x_size);
	terminal.set_height(y_size);
	terminal.theme() = ImTerm::themes::cherry;
	terminal.log_level(ImTerm::message::severity::info);
	terminal.set_flags(ImGuiWindowFlags_NoTitleBar);
	terminal.disallow_x_resize();
	terminal.filter_hint() = utils::gui_text_for("view.terminal.regex_filter");
}
} // namespace

view::view::~view() {
	assert(!m_thread || !m_thread->joinable());
}

void view::view::exec(state::holder &state) {
	assert(!m_thread);

	m_running.test_and_set();
	m_thread = std::make_unique<std::thread>([&]() {
		do_run(state);
	});
}

void view::view::do_run(state::holder &state) {
	constexpr unsigned int x_window_size = 1600;
	constexpr unsigned int y_window_size = 900;

	sf::RenderWindow window{sf::VideoMode{x_window_size, y_window_size}, "Ninja clown !"};
	window.setFramerateLimit(std::numeric_limits<unsigned int>::max());
	window.clear();
	m_window_size = window.getSize();

	ImTerm::terminal<terminal_commands> &terminal = state::access<view>::terminal(state);
	setup_terminal(terminal, x_window_size, y_window_size / 3);

	game_viewer game{window, state};
	map_editor editor{window, state};

	m_game   = &game;
	m_editor = &editor;

	ImGui::SFML::Init(window);
	ImGui::GetIO().IniFilename = nullptr;

	m_fps_limiter.start_now();
	sf::Clock clock{};

	constexpr std::array<ImWchar, 3> fontawesome_icons_ranges = {ICON_MIN_FA, ICON_MAX_FA, 0};
	ImFontConfig fontawesome_icons_config{};
	fontawesome_icons_config.MergeMode        = true;
	fontawesome_icons_config.PixelSnapH       = true;
	fontawesome_icons_config.GlyphMinAdvanceX = 13.5f;

	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->AddFontDefault(&fontawesome_icons_config);

	auto *fontawesome = io.Fonts->AddFontFromFileTTF((utils::resources_directory() / fontawesome_path).generic_string().c_str(), 13.5f,
	                                                 &fontawesome_icons_config, fontawesome_icons_ranges.data());

	if (fontawesome == nullptr) {
		utils::log::warn("view.view.FA_load_failed");
	}
	else {
		utils::log::debug("view.view.FA_load_success");
		ImGui::SFML::UpdateFontTexture();
	}

	while (m_running.test_and_set() && window.isOpen()) {
		manage_events(window, state);
		ImGui::SFML::Update(window, clock.restart());
		auto restore_view = window.getView();

		switch (m_show_state) {
			case window::game:
				if (!game.show(show_debug_data)) {
					m_show_state = window::menu;
				}
				break;
			case window::menu:
				break;
			case window::map_editor:
				if (!editor.show()) {
					m_show_state = window::menu;
				}
				break;
			default:
				utils::log::warn("view.view.bad_state", "state"_a = static_cast<int>(m_show_state));
		}

		if (m_showing_term) {
			ImGui::SetNextWindowPos(ImVec2{0, 0}, ImGuiCond_Always);
			terminal.show();
		}

		ImGui::SFML::Render();

		{ // Fixxy doo fix, fixes linux display bug somehow
			sf::RectangleShape rect;
			rect.setFillColor(sf::Color::Transparent);
			rect.setPosition(-10.f, -10.f);
			window.draw(rect);
		}

		window.setView(restore_view);
		window.display();
		m_fps_limiter.wait();
		window.clear();
	}

	m_game   = nullptr;
	m_editor = nullptr;

	ImGui::SFML::Shutdown();
}

void view::view::manage_events(sf::RenderWindow &window, state::holder &state) noexcept {
	sf::Event event{};
	while (window.pollEvent(event)) {
		if (event.type == sf::Event::KeyPressed) {
			if (event.key.code == sf::Keyboard::F11) {
				m_showing_term = !m_showing_term;
				continue;
			}
		}

		ImGui::SFML::ProcessEvent(event);

		// TODO Also checks for WantCaptureKeyboard (needs refactor event system)
		if (is_mouse_event(event.type) && ImGui::GetIO().WantCaptureMouse) {
			continue;
		}
		if (is_kb_event(event.type) && !is_fallthrough_key(event.key.code) && ImGui::GetIO().WantCaptureKeyboard) {
			continue;
		}


		if (event.type == sf::Event::Closed) {
			window.close();
		}

		if (event.type == sf::Event::Resized) {
			state::access<view>::terminal(state).set_width(window.getSize().x);
		}

		// Re setting correct view port
		if (event.type == sf::Event::Resized) {
			if (m_window_size.x == 0 || m_window_size.y == 0) {
				m_window_size.x = event.size.width;
				m_window_size.y = event.size.height;
			}
			else {

				const sf::Event::SizeEvent sz = event.size;
				const float x_ratio           = static_cast<float>(sz.width) / static_cast<float>(m_window_size.x);
				const float y_ratio           = static_cast<float>(sz.height) / static_cast<float>(m_window_size.y);

				sf::View view               = window.getView();
				const sf::Vector2f top_left = view.getCenter() - view.getSize() / 2.f;
				view.setSize(view.getSize().x * x_ratio, view.getSize().y * y_ratio);
				view.setCenter(top_left + view.getSize() / 2.f);

				m_window_size.x = sz.width;
				m_window_size.y = sz.height;
				window.setView(view);
			}
		}

		// Zooming in and out, moving the map around
		if (m_show_state == window::game || m_show_state == window::map_editor) {
			manage_zoom(event, window);
		}

		switch (m_show_state) {
			case window::game:
				m_game->event(event);
				break;
			case window::menu:
				// TODO : pass events to main menu once it’s developped
				break;
			case window::map_editor:
				m_editor->event(event);
				break;
		}
	}
}

void view::view::manage_zoom(sf::Event event, sf::RenderWindow &window) noexcept {
	switch (event.type) {
		case sf::Event::MouseWheelScrolled: {
			const sf::Event::MouseWheelScrollEvent wheel_scroll = event.mouseWheelScroll;
			if (wheel_scroll.wheel == sf::Mouse::Wheel::VerticalWheel) {
				sf::View view = window.getView();

				const float delta = 1.1f;
				float transform{};
				if (wheel_scroll.delta < 0) {
					transform = delta;
				}
				else {
					transform = 1 / delta;
				}

				const float vp2win_ratio_x = view.getSize().x / static_cast<float>(m_window_size.x);
				const float vp2win_ratio_y = view.getSize().y / static_cast<float>(m_window_size.y);
				const auto wheel_x         = static_cast<float>(wheel_scroll.x);
				const auto wheel_y         = static_cast<float>(wheel_scroll.y);

				const sf::Vector2f top_left
				  = view.getCenter() - view.getSize() / 2.f
				    + sf::Vector2f{wheel_x * (1 - transform) * vp2win_ratio_x, wheel_y * (1 - transform) * vp2win_ratio_y};
				view.zoom(transform);
				view.setCenter(top_left + view.getSize() / 2.f);

				window.setView(view);
			}
			break;
		}

		case sf::Event::MouseMoved: {
			const sf::Event::MouseMoveEvent move = event.mouseMove;
			if (m_middle_click_pos) {
				sf::View view           = window.getView();
				const float zoom_factor = static_cast<float>(window.getSize().x) / view.getSize().x;

				view.move(static_cast<float>(m_mouse_pos.x - move.x) / zoom_factor,
				          static_cast<float>(m_mouse_pos.y - move.y) / zoom_factor);
				window.setView(view);
			}
			m_mouse_pos = {move.x, move.y};
			break;
		}

		case sf::Event::MouseButtonReleased:
			switch (const sf::Event::MouseButtonEvent button = event.mouseButton; button.button) {
				case sf::Mouse::Button::Middle:
					m_middle_click_pos.reset();
					break;
				case sf::Mouse::Button::Right:
					m_right_click_pos.reset();
					break;
				default:
					break;
			}
			break;

		case sf::Event::MouseButtonPressed:
			switch (const sf::Event::MouseButtonEvent button = event.mouseButton; button.button) {
				case sf::Mouse::Button::Middle:
					m_middle_click_pos = {button.x, button.y};
					break;
				case sf::Mouse::Button::Right:
					m_right_click_pos = {button.x, button.y};
					break;
				default:
					break;
			}
			break;

		default:
			break;
	}
}

bool view::view::has_map() const noexcept {
	return m_game != nullptr && m_game->has_map();
}

void view::view::move_entity(adapter::view_handle handle, float new_x, float new_y) noexcept {
	map_viewer().acquire_overmap()->move_entity(handle, new_x, new_y);
}

void view::view::hide(adapter::view_handle handle) noexcept {
	map_viewer().acquire_overmap()->hide(handle);
}

void view::view::reveal(adapter::view_handle handle) noexcept {
	map_viewer().acquire_overmap()->reveal(handle);
}

void view::view::set_map(::view::map_viewer&& map) {
	switch (m_show_state) {
		case window::game:
			game().set_map(std::move(map));
			break;
		case window::menu:
			break;
		case window::map_editor:
			m_editor->set_map(std::move(map));
			break;
	}
}

void view::view::set_tile(unsigned int x, unsigned int y, utils::resources_type::tile_id id) {
	map_viewer().set_tile(x,y,id);
}

adapter::view_handle view::view::add_mob(mob &&mob) {
	return map_viewer().acquire_overmap()->add_mob(std::move(mob));
}

adapter::view_handle view::view::add_object(object &&object) {
	return map_viewer().acquire_overmap()->add_object(std::move(object));
}

void view::view::rotate_entity(adapter::view_handle handle, facing_direction::type dir) noexcept {
	map_viewer().acquire_overmap()->rotate_entity(handle, dir);
}

void view::view::erase(adapter::view_handle handle) noexcept {
	map_viewer().acquire_overmap()->erase(handle);
}

void view::view::set_mob_kind(adapter::view_handle handle, adapter::entity_prop::behaviour bhvr) noexcept {
	map_viewer().acquire_overmap()->set_mob_kind(handle, mob_id_from_behaviour(bhvr));
}

utils::resources_type::mob_id view::view::mob_id_from_behaviour(adapter::entity_prop::behaviour ibhvr) {
	using bhvr = adapter::entity_prop::behaviour::bhvr;

	switch (ibhvr.val) {
		case bhvr::harmless:
			return utils::resources_type::mob_id::harmless;
		case bhvr::patrol:
			return utils::resources_type::mob_id::patrol;
		case bhvr::aggressive:
			return utils::resources_type::mob_id::aggressive;
		case bhvr::dll:
			return utils::resources_type::mob_id::dll;
		default:
			utils::log::warn("view.view.set_mob_kind.unsupported_behaviour", "bhvr"_a = static_cast<int>(ibhvr.val));
			return utils::resources_type::mob_id::harmless;
	}
}

view::map_viewer& view::view::map_viewer() {
	switch (m_show_state) {
		case window::game:
			return game().get_map();
		case window::menu:
			break;
		case window::map_editor:
			return m_editor->get_map();
	}

	throw std::runtime_error(utils::gui_str_for("view.view.invalid_state"));
}
std::vector<std::vector<view::cell>> view::view::get_cells() {
	return map_viewer().acquire_cellmap()->get_cells();
}
