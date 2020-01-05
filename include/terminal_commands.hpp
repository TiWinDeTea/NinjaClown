#ifndef NINJACLOWN_TERMINAL_COMMANDS_HPP
#define NINJACLOWN_TERMINAL_COMMANDS_HPP

#include <array>
#include <initializer_list>
#include <mutex>
#include <string>
#include <vector>

#include <imterm/terminal.hpp>
#include <imterm/terminal_helpers.hpp>

struct empty_struct {
};
class terminal_commands: public ImTerm::basic_spdlog_terminal_helper<terminal_commands, empty_struct, misc::no_mutex> {
public:
	terminal_commands();

	static std::vector<std::string> no_completion(argument_type &)
	{
		return {};
	}

	static void clear(argument_type &);
	static void echo(argument_type &);
	static void exit(argument_type &);
	static void help(argument_type &);
	static void quit(argument_type &);
	static void load_shared_library(argument_type &);
	static void load_map(argument_type &);
	static void set_fps(argument_type &);
	static void valueof(argument_type &);

	static std::vector<std::string> autocomplete_path(argument_type &, const std::initializer_list<std::string_view> &extensions);
	static std::vector<std::string> autocomplete_variable(argument_type &);
};

#endif // NINJACLOWN_TERMINAL_COMMANDS_HPP
