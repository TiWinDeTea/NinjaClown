#include "terminal_commands.hpp"
#include "bot/bot_dll.hpp"
#include "bot/bot_factory.hpp"
#include "bot_interface/bot.h"
#include "model/world.hpp"
#include "utils/utils.hpp"

#include <imterm/misc.hpp>

#include <array>
#include <charconv>
#include <filesystem>
#include <program_state.hpp>

namespace {
std::vector<std::string> autocomplete_library_path(terminal_commands::argument_type &arg)
{
	return terminal_commands::autocomplete_path(arg, {".so", ".dll"});
}

std::vector<std::string> autocomplete_map_path(terminal_commands::argument_type &arg)
{
	return terminal_commands::autocomplete_path(arg, {".map"});
}

constexpr std::array local_command_list{
  terminal_commands::command_type{"clear", "clears the terminal screen", terminal_commands::clear, terminal_commands::no_completion},
  terminal_commands::command_type{"echo", "prints text", terminal_commands::echo, terminal_commands::no_completion},
  terminal_commands::command_type{"exit", "closes this terminal", terminal_commands::exit, terminal_commands::no_completion},
  terminal_commands::command_type{"help", "show this help", terminal_commands::help, terminal_commands::no_completion},
  terminal_commands::command_type{"print", "prints text", terminal_commands::echo, terminal_commands::no_completion},
  terminal_commands::command_type{"quit", "closes this application", terminal_commands::quit, terminal_commands::no_completion},
  terminal_commands::command_type{"load_dll", "loads a shared library", terminal_commands::load_shared_library, autocomplete_library_path},
  terminal_commands::command_type{"load_map", "loads map from file", terminal_commands::load_map, autocomplete_map_path},
  terminal_commands::command_type{"update_world", "update world once", terminal_commands::update_world, terminal_commands::no_completion},
  terminal_commands::command_type{"set", "sets a variable", terminal_commands::set,
                                  terminal_commands::autocomplete_variable}, // TODO: autocomplete only settable variable
  terminal_commands::command_type{"valueof", "prints the value of a game variable", terminal_commands::valueof,
                                  terminal_commands::autocomplete_variable}}; // TODO:autocomplete with map
} // namespace

terminal_commands::terminal_commands()
{
	for (const command_type &cmd : local_command_list) {
		add_command_(cmd);
	}
}

void terminal_commands::clear(argument_type &arg)
{
	arg.term.clear();
}

void terminal_commands::echo(argument_type &arg)
{
	if (arg.command_line.size() < 2) {
		arg.term.add_text("");
		return;
	}
	if (arg.command_line[1][0] == '-') {
		if (arg.command_line[1] == "--help" || arg.command_line[1] == "-help") {
			arg.term.add_text("usage: " + arg.command_line[0] + "[text to be printed]");
		}
		else {
			arg.term.add_text("Unknown argument: " + arg.command_line[1]);
		}
	}
	else {
		std::string str{};
		auto it = std::next(arg.command_line.begin(), 1);
		while (it != arg.command_line.end() && it->empty()) {
			++it;
		}
		if (it != arg.command_line.end()) {
			str = *it;
			for (++it; it != arg.command_line.end(); ++it) {
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

void terminal_commands::exit(argument_type &arg)
{
	arg.term.set_should_close();
}

void terminal_commands::help(argument_type &arg)
{
	constexpr static unsigned long list_element_name_max_size
	  = misc::max_size(local_command_list.begin(), local_command_list.end(), [](const command_type &cmd) {
		    return cmd.name.size();
	    });

	auto add_cmd = [&arg](std::string_view command_name, std::string_view description) {
		std::string str;
		str.reserve(12 + list_element_name_max_size + description.size());
		str = "        ";
		for (unsigned int i = list_element_name_max_size; i > command_name.size(); --i) {
			str += ' ';
		}
		str += command_name;
		str += " | ";
		str += description;
		arg.term.add_text(str);
	};

	arg.term.add_text("Available commands:");
	for (const command_type &cmd : local_command_list) {
		add_cmd(cmd.name, cmd.description);
	}
	arg.term.add_text("");
	arg.term.add_text("Additional information might be available using \"'command' --help\"");
}

void terminal_commands::quit(argument_type &)
{
	program_state::global->close_request = true;
}

void terminal_commands::load_shared_library(argument_type &arg)
{
	if (arg.command_line.size() < 2) {
		arg.term.add_text("usage: " + arg.command_line[0] + " <shared_library_path>");
		return;
	}

	std::string shared_library_path = arg.command_line[1];
	arg.term.add_text("loading " + shared_library_path);

	if (program_state::global->bot_dll.load(shared_library_path)) {
		program_state::global->bot_dll.bot_init(bot::make_api());
	}
	else {
		arg.term.add_text("error loading dll");
	}
}

void terminal_commands::load_map(argument_type &arg)
{
	if (arg.command_line.size() < 2) {
		arg.term.add_text("usage: " + arg.command_line[0] + " <map_path>");
		return;
	}

	program_state::global->adapter.load_map(arg.command_line[1]);
}

void terminal_commands::update_world(argument_type &arg)
{
	program_state::global->bot_dll.bot_think();
	program_state::global->world.update();
}

void terminal_commands::set(argument_type &arg)
{
    auto show_usage = [&](){
        arg.term.add_formatted("usage: {} <variable> <integer value>", arg.command_line.front());
    };

    // todo: std::map & lower_bound + higher_bound, ptr vers fonctions
	if (arg.command_line.size() == 3) {
        auto value = utils::from_chars<int>(arg.command_line.back());
	    if ("display_debug_data" == arg.command_line[1]) {
	        if (value && *value != 0 && *value != 1) {
				arg.term.add_text("display_debug_data must be either 0 (no) or 1 (yes)");
			} else {
	            if (value) {
					program_state::global->viewer.show_debug_data = (*value == 1);
				} else {
                    if (arg.command_line.back() == "true") {
                        program_state::global->viewer.show_debug_data = true;
                    } else if (arg.command_line.back() == "false") {
                        program_state::global->viewer.show_debug_data = false;
                    } else {
                        show_usage();
                    }
	            }
	        }
            return;
	    }
	    if ("target_fps" == arg.command_line[1]) {
            if (!value) {
                show_usage();
                return;
            }
			if (*value < 0) {
			    arg.term.add_text("target_fps must be positive");
			} else {
				program_state::global->viewer.target_fps(*value);
			}
            return;
		}
	    if ("average_fps" == arg.command_line[1]) {
	        arg.term.add_text("average_fps can not be set");
	        return;
	    }
	}

	show_usage();

}

void terminal_commands::valueof(argument_type &arg)
{
	// todo: std::map & lower_bound + higher_bound, ptr vers fonctions
	if (arg.command_line.size() == 2) {
		if ("average_fps" == arg.command_line.back()) {
			arg.term.add_formatted("average fps: {:.1f}", program_state::global->viewer.average_fps());
			return;
		}
		if ("target_fps" == arg.command_line.back()) {
			arg.term.add_formatted("max fps: {}", program_state::global->viewer.target_fps());
			return;
		}
		if ("display_debug_data" == arg.command_line.back()) {
		    arg.term.add_formatted("display debug data: {}", program_state::global->viewer.show_debug_data.load());
		    return;
		}
	}

	arg.term.add_formatted("usage: {} <variable>", arg.command_line.front());
}

std::vector<std::string> terminal_commands::autocomplete_path(argument_type &arg, const std::initializer_list<std::string_view> &extensions)
{
	auto escape = [](std::string &&str) -> std::string && {
		std::string::size_type pos = str.find('\\');
		while (pos != std::string::npos) {
			str[pos] = '/';
			pos      = str.find('\\', pos);
		}
		return std::move(str);
	};

	std::vector<std::string> paths;

	if (arg.command_line.size() == 2) {
		std::string_view current = arg.command_line[1];
		std::string_view file_prefix;
		std::string_view directory;

		if (auto last_dir_sep = current.find_last_of("/\\"); last_dir_sep != std::string_view::npos) {
			file_prefix = current.substr(last_dir_sep + 1);
			directory   = current.substr(0, last_dir_sep);
			if (directory.empty()) {
				directory = "/";
			}
		}
		else {
			directory   = ".";
			file_prefix = current;
		}

		std::error_code ec;
		std::filesystem::directory_iterator d_it(directory, ec);
		if (!ec) {
			unsigned int simple_file_count{0};
			for (const auto &entry : d_it) {
				if (utils::starts_with(entry.path().filename().string(), file_prefix)) {

					const auto &ext = entry.path().extension();
					if (entry.is_directory()) {
						paths.emplace_back(escape(entry.path().string() + "/"));
					}
					else {
						for (const std::string_view &str : extensions) {
							if (ext == str) {
								paths.emplace_back(escape(entry.path().string()));
								std::swap(paths[simple_file_count++], paths.back());
								break;
							}
						}
					}
				}
			}
		}
	}
	return paths;
}
std::vector<std::string> terminal_commands::autocomplete_variable(argument_type &arg)
{
	std::vector<std::string> ans;
	if (arg.command_line.size() != 2) {
		return ans;
	}

	if (utils::starts_with("average_fps", arg.command_line.back())) { // todo: std::map + lower_bound & higher_bound
		ans.emplace_back("average_fps");
	}
	if (utils::starts_with("target_fps", arg.command_line.back())) {
		ans.emplace_back("target_fps");
	}
	if (utils::starts_with("display_debug_data", arg.command_line.back())) {
	    ans.emplace_back("display_debug_data");
	}
	return ans;
}
