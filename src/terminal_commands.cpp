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
  terminal_commands::command_type{"fps_max", "sets the target fps", terminal_commands::set_fps,
                                  terminal_commands::no_completion}, // TODO: autocomplete with current value
  terminal_commands::command_type{"valueof", "prints the value of a game variable", terminal_commands::valueof,
                                  terminal_commands::autocomplete_variable}};
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

void terminal_commands::quit(argument_type &arg)
{
	arg.val.close_request = true;
}

void terminal_commands::load_shared_library(argument_type &arg)
{
	if (arg.command_line.size() < 2) {
		arg.term.add_text("usage: " + arg.command_line[0] + " <shared_library_path>");
		return;
	}

	std::string shared_library_path = arg.command_line[1];
	arg.term.add_text("loading " + shared_library_path);

	bot::bot_dll bot{shared_library_path};
	if (!bot) {
		arg.term.add_text("couldn't load bot library: " + bot.error());
		return;
	}
	else {
		bot::bot_api api = bot::make_api(bot::go_right{&bot::ffi::go_right});
		bot.bot_init(&api);
		bot.bot_think();
	}

	if (bot.reload()) {
		bot::bot_api api = bot::make_api(bot::go_right{&bot::ffi::go_right_dummy});
		bot.bot_init(&api);
		bot.bot_think();
	}
}

void terminal_commands::load_map(argument_type &arg)
{
	if (arg.command_line.size() < 2) {
		arg.term.add_text("usage: " + arg.command_line[0] + " <map_path>");
		return;
	}

	model::world world{arg.command_line[1]};
	world.update();
}

void terminal_commands::set_fps(argument_type &arg)
{
	if (arg.command_line.size() == 2) {
		auto value = utils::from_chars<unsigned int>(arg.command_line.back());
		if (value) {
			arg.val.viewer.target_fps(*value);
			return;
		}
	}

	arg.term.add_formatted("usage: {} <unsigned integer value>", arg.command_line.front());
}

void terminal_commands::valueof(argument_type &arg)
{
	// todo: std::map & lower_bound + higher_bound, ptr vers fonctions membres
	if (arg.command_line.size() == 2) {
		if ("average_fps" == arg.command_line.back()) {
			arg.term.add_formatted("average fps: {:.1f}", arg.val.viewer.average_fps());
			return;
		}
		if ("target_fps" == arg.command_line.back()) {
			arg.term.add_formatted("max fps: {}", arg.val.viewer.target_fps());
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
	return ans;
}
