#include "terminal_commands.hpp"

#include <imterm/misc.hpp>

#include <array>
#include <charconv>
#include <dlfcn.h> // linux

namespace {

	constexpr std::array local_command_list{
			terminal_commands::command_type{"clear", "clears the terminal screen", terminal_commands::clear,
			                                terminal_commands::no_completion},
			terminal_commands::command_type{"echo", "prints text", terminal_commands::echo,
			                                terminal_commands::no_completion},
			terminal_commands::command_type{"exit", "closes this terminal", terminal_commands::exit,
			                                terminal_commands::no_completion},
			terminal_commands::command_type{"help", "show this help", terminal_commands::help,
			                                terminal_commands::no_completion},
			terminal_commands::command_type{"print", "prints text", terminal_commands::echo,
			                                terminal_commands::no_completion},
			terminal_commands::command_type{"quit", "closes this application", terminal_commands::quit,
			                                terminal_commands::no_completion},
			terminal_commands::command_type{"load", "loads a shared library", terminal_commands::load_shared_library,
								            terminal_commands::no_completion},
	};
}

terminal_commands::terminal_commands() {
	for (const command_type& cmd : local_command_list) {
		add_command_(cmd);
	}
}

void terminal_commands::clear(argument_type& arg) {
	arg.term.clear();
}

void terminal_commands::echo(argument_type& arg) {
	if (arg.command_line.size() < 2) {
		arg.term.add_text("");
		return;
	}
	if (arg.command_line[1][0] == '-') {
		if (arg.command_line[1] == "--help" || arg.command_line[1] == "-help") {
			arg.term.add_text("usage: " + arg.command_line[0] + "[text to be printed]");
		} else {
			arg.term.add_text("Unknown argument: " + arg.command_line[1]);
		}
	} else {
		std::string str{};
		auto it = std::next(arg.command_line.begin(), 1);
		while (it != arg.command_line.end() && it->empty()) {
			++it;
		}
		if (it != arg.command_line.end()) {
			str = *it;
			for (++it ; it != arg.command_line.end() ; ++it) {
				if (it->empty()) {
					continue;
				}
				str.reserve(str.size() + it->size() + 1);
				str += ' ';
				str += *it;
			}
		}
		arg.term.add_text(str);
	}
}

void terminal_commands::exit(argument_type& arg) {
	arg.term.set_should_close();
}

void terminal_commands::help(argument_type& arg) {
	constexpr static unsigned long list_element_name_max_size = misc::max_size(local_command_list.begin(), local_command_list.end(),
	                                                                    [](const command_type& cmd) { return cmd.name.size(); });

	auto add_cmd = [&arg](std::string_view command_name, std::string_view description) {
		std::string str;
		str.reserve(12 + list_element_name_max_size + description.size());
		str = "        ";
		for (unsigned int i = list_element_name_max_size ; i > command_name.size() ; --i) {
			str += ' ';
		}
		str += command_name;
		str += " | ";
		str += description;
		arg.term.add_text(str);
	};

	arg.term.add_text("Available commands:");
	for (const command_type& cmd : local_command_list) {
		add_cmd(cmd.name, cmd.description);
	}
	arg.term.add_text("");
	arg.term.add_text("Additional information might be available using \"'command' --help\"");
}

void terminal_commands::quit(argument_type& arg) {
	arg.val.close_request = true;
}

void terminal_commands::load_shared_library(argument_type& arg) {
	using think_fn_type = int(*)(int);

	if (arg.command_line.size() < 3) {
		arg.term.add_text("usage: " + arg.command_line[0] + " <shared_library_path> <some_magic_int>");
		return;
	}

	int think_param;
	{
		std::string think_param_str = arg.command_line[2];
		auto [p, ec] = std::from_chars(think_param_str.data(), think_param_str.data()+think_param_str.size(), think_param);
		if (ec != std::errc()) {
			arg.term.add_text("couldn't parse magic int");
			return;
		}
	}

	std::string shared_library_path = arg.command_line[1];
	arg.term.add_text("loading " + shared_library_path);

	void* handle = dlopen(shared_library_path.c_str(), RTLD_NOW);
	if (!handle) {
		arg.term.add_text(std::string{dlerror()});
		return;
	}

	auto think = reinterpret_cast<think_fn_type>(dlsym(handle, "think"));
	char* error = dlerror();
	if (error != nullptr)  {
		arg.term.add_text(std::string{error});
		return;
	}

	int think_result = think(think_param);
	arg.term.add_text("thinking about " + std::to_string(think_result));

	dlclose(handle);
}
