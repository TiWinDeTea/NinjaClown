#include "terminal_commands.hpp"
#include "bot/bot_dll.hpp"
#include "bot/bot_factory.hpp"
#include "bot_interface/bot.h"
#include "model/world.hpp"
#include "state_holder.hpp"
#include "utils/resource_manager.hpp"
#include "utils/utils.hpp"

#include <imterm/misc.hpp>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

#include <array>
#include <charconv>
#include <filesystem>
#include <utils/visitor.hpp>

// TODO translations

namespace {
std::vector<std::string> autocomplete_library_path(terminal_commands::argument_type &arg) {
	return terminal_commands::autocomplete_path(arg, {".so", ".dll"});
}

std::vector<std::string> autocomplete_map_path(terminal_commands::argument_type &arg) {
	return terminal_commands::autocomplete_path(arg, {".map"});
}

std::vector<std::string> autocomplete_config(terminal_commands::argument_type &arg) {
	return terminal_commands::autocomplete_path(arg, {".toml"});
}

struct cmd {
	command_id cmd;
	void (*cmd_ptr)(terminal_commands::argument_type &);
	std::vector<std::string> (*complete_ptr)(terminal_commands::argument_type &);
};

constexpr std::array local_command_list{
  cmd{command_id::clear, &terminal_commands::clear, terminal_commands::no_completion},
  cmd{command_id::echo, terminal_commands::echo, terminal_commands::no_completion},
  cmd{command_id::exit, terminal_commands::exit, terminal_commands::no_completion},
  cmd{command_id::help, terminal_commands::help, terminal_commands::no_completion},
  cmd{command_id::print, terminal_commands::echo, terminal_commands::no_completion},
  cmd{command_id::quit, terminal_commands::quit, terminal_commands::no_completion},
  cmd{command_id::load_shared_library, terminal_commands::load_shared_library, autocomplete_library_path},
  cmd{command_id::load_map, terminal_commands::load_map, autocomplete_map_path},
  cmd{command_id::update_world, terminal_commands::update_world, terminal_commands::no_completion},
  cmd{command_id::set, terminal_commands::set, terminal_commands::autocomplete_variable}, // TODO: autocomplete only settable variable
  cmd{command_id::valueof, terminal_commands::valueof, terminal_commands::autocomplete_variable}, // TODO:autocomplete with map
  cmd{command_id::reconfigure, terminal_commands::reconfigure, autocomplete_config},
  cmd{command_id::fire_actionable, terminal_commands::fire_actionable, terminal_commands::no_completion},
  cmd{command_id::fire_activator, terminal_commands::fire_activator, terminal_commands::no_completion}};

bool as_bool(std::string_view str) {
	std::optional<int> int_val = utils::from_chars<int>(str);
	if (int_val) {
		return *int_val != 0;
	}
	else {
		return str == "true";
	}
}
} // namespace

void terminal_commands::load_commands(const utils::resource_manager &resources) noexcept {
	cmd_list_.clear();
	for (const auto &cmd : local_command_list) {

		auto maybe_command = resources.text_for(cmd.cmd);
		if (!maybe_command) {
			std::string error = "No name found for command " + std::to_string(static_cast<int>(cmd.cmd));
			spdlog::warn(error);
		}

		auto [name, desc] = *maybe_command;

		command_type command;
		command.call        = cmd.cmd_ptr;
		command.complete    = cmd.complete_ptr;
		command.description = desc;
		command.name        = name;
		add_command_(command);
	}
}

void terminal_commands::set_terminal(term_t &term) noexcept {
	terminal_ = &term;
	for (ImTerm::message &msg : m_pre_logged_messages) {
		term.add_message(std::move(msg));
	}
	m_pre_logged_messages.clear();
}

void terminal_commands::sink_it_(const spdlog::details::log_msg &msg) {
	if (msg.level == spdlog::level::off) {
		return;
	}
	spdlog::memory_buf_t buff{};
	spdlog::sinks::base_sink<misc::no_mutex>::formatter_->format(msg, buff);

	ImTerm::message imsg{ImTerm::details::to_imterm_severity(msg.level), fmt::to_string(buff), msg.color_range_start, msg.color_range_end,
	                     false};
	if (terminal_ != nullptr) {
		terminal_->add_message(std::move(imsg));
	}
	else {
		m_pre_logged_messages.emplace_back(std::move(imsg));
	}
}

void terminal_commands::clear(argument_type &arg) {
	arg.term.clear();
}

void terminal_commands::echo(argument_type &arg) {
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

void terminal_commands::exit(argument_type &arg) {
	arg.term.set_should_close();
}

void terminal_commands::help(argument_type &arg) {
	std::vector<std::pair<std::string_view, std::string_view>> commands; // name, description
	for (const auto &cmd : local_command_list) {
		auto opt = arg.val.resources.text_for(cmd.cmd);
		if (!opt) {
			spdlog::warn("No name found for command {}", static_cast<int>(cmd.cmd));
			continue;
		}
		auto [name, desc] = *opt;
		commands.emplace_back(name, desc);
	}

	const unsigned int element_max_name_size = misc::max_size(commands.begin(), commands.end(), [](const auto &str) {
		return str.first.size();
	});

	auto add_cmd = [&arg, &element_max_name_size](std::string_view command_name, std::string_view description) {
		std::string str;
		str.reserve(12 + element_max_name_size + description.size());
		str = "        ";
		for (unsigned int i = element_max_name_size; i > command_name.size(); --i) {
			str += ' ';
		}
		str += command_name;
		str += " | ";
		str += description;
		arg.term.add_text(str);
	};

	arg.term.add_text("Available commands:");
	for (const auto &cmd : commands) {
		add_cmd(cmd.first, cmd.second);
	}

	arg.term.add_text("");
	arg.term.add_text("Additional information might be available using \"'command' --help\""); // todo externaliser
}

void terminal_commands::quit(argument_type &arg) {
	arg.val.m_view.close_requested = true;
}

void terminal_commands::load_shared_library(argument_type &arg) {
	if (arg.command_line.size() < 2) {
		arg.term.add_text("usage: " + arg.command_line[0] + " <shared_library_path>"); // todo externaliser
		return;
	}

	std::string shared_library_path = arg.command_line[1];
	arg.term.add_text("loading " + shared_library_path); // todo externaliser

	if (!arg.val.m_model.load_dll(shared_library_path)) {
		arg.term.add_text("error loading dll"); // todo externaliser
	}
}

void terminal_commands::load_map(argument_type &arg) {
	if (arg.command_line.size() < 2) {
		arg.term.add_text("usage: " + arg.command_line[0] + " <map_path>");
		return;
	}

	if (arg.val.m_adapter.map_is_loaded())
		arg.val.m_model.bot_end_level();

	arg.val.m_adapter.load_map(arg.command_line[1]);
	arg.val.m_model.bot_start_level(bot::make_api());
}

void terminal_commands::update_world(argument_type &arg) {
	arg.val.m_model.bot_think();
	arg.val.m_adapter.m_cells_changed_since_last_update.clear();
	arg.val.m_model.world.update(arg.val.m_adapter);
}

void terminal_commands::run_model(argument_type &arg) {
	arg.val.m_model.run();
}

void terminal_commands::stop_model(argument_type &arg) {
	arg.val.m_model.stop();
}

void terminal_commands::set(argument_type &arg) {
	if (arg.command_line.size() == 3) {
		auto it = arg.val.properties.find(arg.command_line[1]);
		if (it == arg.val.properties.end()) {
			arg.term.add_formatted_err("Unknown variable : {}", arg.command_line[1]); // TODO externaliser
			return;
		}

		utils::visitor property_visitor{[&](state::property::proxy<unsigned int> &p) {
			                                std::optional<unsigned int> value = utils::from_chars<unsigned int>(arg.command_line.back());
			                                if (value) {
				                                p.set(*value);
			                                }
			                                else {
				                                arg.term.add_formatted_err("{} is not an unsigned integer.",
				                                                           arg.command_line.back()); // TODO externaliser
			                                }
		                                },
		                                [&](state::property::proxy<std::atomic_bool> &p) {
			                                p.set(as_bool(arg.command_line.back()));
		                                }};
		utils::visitor property_dispatcher{[&](const state::property::readonly_property & /* ignored*/) {
			                                   arg.term.add_formatted_err("Error: {} is read-only",
			                                                              arg.command_line.back()); // TODO externaliser
		                                   },
		                                   [&](state::property::settable_property p) {
			                                   std::visit(property_visitor, p);
		                                   }};

		std::visit(property_dispatcher, it->second.data());
	}
	else {
		arg.term.add_formatted("usage: {} <variable> <value>", arg.command_line.front()); // TODO externaliser
	}
}

void terminal_commands::valueof(argument_type &arg) {
	if (arg.command_line.size() == 2) {
		auto it = arg.val.properties.find(arg.command_line.back());
		if (it == arg.val.properties.end()) {
			arg.term.add_formatted_err("Unknown variable : {}", arg.command_line.back()); // TODO externaliser
			return;
		}

		auto display_var = [&](const auto &var) {
			arg.term.add_formatted("{} : {}", arg.command_line.back(), var);
		};
		utils::visitor property_visitor{[&](const state::property::proxy<unsigned int> &p) {
			                                display_var(p.get());
		                                },
		                                [&](const state::property::proxy<std::atomic_bool> &p) {
			                                display_var(p.get());
		                                },
		                                [&](float flt) {
			                                display_var(flt);
		                                }};
		utils::visitor property_dispatcher{[&](const state::property::readonly_property &p) {
			                                   std::visit(property_visitor, p);
		                                   },
		                                   [&](const state::property::settable_property &p) {
			                                   std::visit(property_visitor, p);
		                                   }};

		std::visit(property_dispatcher, it->second.data());
	}
	else {
		arg.term.add_formatted("usage: {} <variable>", arg.command_line.front()); // TODO externaliser
	}
}

void terminal_commands::reconfigure(argument_type &arg) {
	if (arg.command_line.size() != 2) {
		arg.term.add_formatted("usage: {} <config file>", arg.command_line.front()); // TODO externaliser
		return;
	}

	if (!arg.val.resources.reload(arg.command_line.back())) {
		arg.term.add_formatted("failed to reload resources from {}", arg.command_line.back()); // TODO externaliser
	}
	else {
		arg.term.add_formatted("{}: resources successfully reloaded", arg.command_line.back()); // TODO externaliser
		arg.val.m_view.reload_sprites();
		arg.val.m_terminal.get_terminal_helper()->load_commands(arg.val.resources);
	}
}

void terminal_commands::fire_activator(argument_type &arg) {
	if (arg.command_line.size() != 2) {
		arg.term.add_formatted("usage: {} <activator id>", arg.command_line.front()); // TODO externaliser
		return;
	}

	std::optional<unsigned int> val = utils::from_chars<unsigned int>(arg.command_line[1]);
	if (!val) {
		arg.term.add_formatted("activator id should be an integer (got {})", arg.command_line.back()); // TODO externaliser
		return;
	}

	if (*val > arg.val.m_model.world.activators.size()) {
		arg.term.add_formatted("Invalid value (got {}, max {})", arg.command_line.back(),
		                       arg.val.m_model.world.activators.size()); // TODO externaliser
		return;
	}

	arg.val.m_model.world.fire_activator(arg.val.m_adapter, *val);
}

void terminal_commands::fire_actionable(argument_type &arg) {
	if (arg.command_line.size() != 2) {
		arg.term.add_formatted("usage: {} <actionable id>", arg.command_line.front()); // TODO externaliser
		return;
	}

	std::optional<unsigned int> val = utils::from_chars<unsigned int>(arg.command_line[1]);
	if (!val) {
		arg.term.add_formatted("activator id should be an integer (got {})", arg.command_line.back()); // TODO externaliser
		return;
	}

	if (*val > arg.val.m_model.world.actionables.size()) {
		arg.term.add_formatted("Invalid value (got {}, max {})", arg.command_line.back(),
		                       arg.val.m_model.world.actionables.size()); // TODO externaliser
		return;
	}
	arg.val.m_model.world.fire_actionable(arg.val.m_adapter, *val);
}

std::vector<std::string> terminal_commands::autocomplete_path(argument_type &arg,
                                                              const std::initializer_list<std::string_view> &extensions) {
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
std::vector<std::string> terminal_commands::autocomplete_variable(argument_type &arg) {
	std::vector<std::string> ans;
	if (arg.command_line.size() != 2) {
		return ans;
	}

	auto it = arg.val.properties.lower_bound(arg.command_line.back());
	while (it != arg.val.properties.end() && utils::starts_with(it->first, arg.command_line.back())) {
		ans.emplace_back(it++->first);
	}
	return ans;
}
