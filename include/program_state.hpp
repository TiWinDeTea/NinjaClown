#ifndef NINJACLOWN_PROGRAM_STATE_HPP
#define NINJACLOWN_PROGRAM_STATE_HPP

#include "bot/bot_dll.hpp"
#include "model/world.hpp"
#include "terminal_commands.hpp"
#include "utils/resource_manager.hpp"
#include "view/viewer.hpp"

#include <imterm/terminal.hpp>

struct program_state {
	static struct empty_struct s_empty;

	ImTerm::terminal<terminal_commands> terminal{s_empty, "terminal"};
	view::viewer viewer{};

	utils::resource_manager resource_manager{};

	model::world world{};

	bot::bot_dll bot_dll{};

	bool close_request{false};
	bool term_on_display{true};

	static program_state *global;
};

#endif //NINJACLOWN_PROGRAM_STATE_HPP
