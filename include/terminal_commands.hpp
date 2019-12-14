#ifndef NINJACLOWN_TERMINAL_COMMANDS_HPP
#define NINJACLOWN_TERMINAL_COMMANDS_HPP

#include <array>
#include <string>
#include <vector>
#include <mutex>

#include <imterm/terminal.hpp>
#include <imterm/terminal_helpers.hpp>

#include <program_state.hpp>

class terminal_commands : public ImTerm::basic_spdlog_terminal_helper<terminal_commands, program_state, misc::no_mutex> {
public:

	terminal_commands();

	static std::vector<std::string> no_completion(argument_type&) { return {}; }

	static void clear(argument_type&);
	static void echo(argument_type&);
	static void exit(argument_type&);
	static void help(argument_type&);
	static void quit(argument_type&);
	static void load_shared_library(argument_type&);
};

#endif // NINJACLOWN_TERMINAL_COMMANDS_HPP
