#include <imgui-SFML.h>
#include <imgui.h>

#include <imterm/terminal.hpp>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include "program_state.hpp"
#include "terminal_commands.hpp"
#include <spdlog/spdlog.h>

namespace cst {
constexpr unsigned long window_width  = 800;
constexpr unsigned long window_height = 450;
} // namespace cst

int main(int argc, char *argv[])
{
	program_state prgm;
	ImTerm::terminal<terminal_commands> terminal_log(prgm, "terminal", cst::window_width);
	terminal_log.theme() = ImTerm::themes::cherry;
	terminal_log.log_level(ImTerm::message::severity::info);
	terminal_log.set_flags(ImGuiWindowFlags_NoTitleBar);
	terminal_log.disallow_x_resize();
	terminal_log.filter_hint() = "regex filter...";

	spdlog::default_logger()->sinks().push_back(terminal_log.get_terminal_helper());
	spdlog::default_logger()->set_level(spdlog::level::trace);

	spdlog::info("~ Ninja. Clown. ~");

	sf::Clock deltaClock;
	sf::Clock musicOffsetClock;

	sf::RenderWindow window({cst::window_width, cst::window_height}, "Ninja clown !");
	bool resized_once = false;
	window.setFramerateLimit(60);
	ImGui::SFML::Init(window);

	ImGuiIO &io    = ImGui::GetIO();
	io.IniFilename = nullptr;

	while (window.isOpen()) {
		sf::Event event{};
		while (window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);

			if (event.type == sf::Event::Closed) {
				window.close();
			}
			else if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::F11) {
					prgm.term_on_display = !prgm.term_on_display;
				}
			}
			else if (event.type == sf::Event::Resized) {
				terminal_log.set_width(window.getSize().x);
				if (resized_once) {
					terminal_log.set_height(std::min(window.getSize().y, static_cast<unsigned>(terminal_log.get_size().y)));
				}
				resized_once = true;
			}
		}

		ImGui::SFML::Update(window, deltaClock.restart());

		if (prgm.term_on_display) {
			ImGui::SetNextWindowPos({0.f, 0.f}, ImGuiCond_Always);
			prgm.term_on_display = terminal_log.show();
			if (prgm.close_request) {
				window.close();
			}
		}

		window.clear();
		ImGui::SFML::Render(window);
		window.display();
	}
	ImGui::SFML::Shutdown();
}