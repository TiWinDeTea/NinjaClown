#include "terminal_commands.hpp"

#include <array>
#include <filesystem>

#include <ninja_clown/api.h>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "bot/bot_api.hpp"
#include "bot/bot_dll.hpp"
#include "model/model.hpp"
#include "model/world.hpp"
#include "state_holder.hpp"
#include "utils/logging.hpp"
#include "utils/resource_manager.hpp"
#include "utils/utils.hpp"
#include "utils/visitor.hpp"
#include "view/viewer.hpp"

using fmt::literals::operator""_a;

namespace {
std::string log_get_or_gen(const terminal_commands::argument_type &arg, std::string_view key) {
	return utils::log::get_or_gen(arg.val.resources(), key);
}

template <typename... Args>
void log_formatted(terminal_commands::argument_type &arg, std::string_view key, Args &&... args) {
	std::string fmt = log_get_or_gen(arg, key);
	try {
		arg.term.add_formatted(fmt.c_str(), std::forward<Args>(args)...);
	}
	catch (const fmt::format_error &error) {
		spdlog::error(R"("{}" while formatting string "{}")", error.what(), fmt);
	}
}

template <typename... Args>
void log_formatted_err(terminal_commands::argument_type &arg, std::string_view key, Args &&... args) {
	std::string fmt = log_get_or_gen(arg, key);
	try {
		arg.term.add_formatted_err(fmt.c_str(), std::forward<Args>(args)...);
	}
	catch (const fmt::format_error &error) {
		spdlog::error(R"("{}" while formatting string "{}")", error.what(), fmt);
	}
}

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
			utils::log::warn(resources, "terminal_commands.command_missing_name", "cmd_id"_a = static_cast<int>(cmd.cmd));
			// continue;
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

void terminal_commands::exit(argument_type &arg) {
	arg.term.set_should_close();
}

void terminal_commands::help(argument_type &arg) {
	std::vector<std::pair<std::string_view, std::string_view>> commands; // name, description
	for (const auto &cmd : local_command_list) {
		auto opt = arg.val.resources().text_for(cmd.cmd);
		if (!opt) {
			utils::log::warn(arg.val.resources(), "terminal_commands.command_missing_name", "cmd_id"_a = static_cast<int>(cmd.cmd));
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

	arg.term.add_text(log_get_or_gen(arg, "terminal_commands.general.help"));
	for (const auto &cmd : commands) {
		add_cmd(cmd.first, cmd.second);
	}
	arg.term.add_text("");
}

void terminal_commands::quit(argument_type &arg) {
	arg.val.view().close_requested = true;
}

void terminal_commands::load_shared_library(argument_type &arg) {
	if (arg.command_line.size() < 2) {
		log_formatted_err(arg, "terminal_commands.load_shared_library.usage", "arg0"_a = arg.command_line[0]);
		return;
	}

	const std::string &shared_library_path = arg.command_line[1];
	log_formatted(arg, "terminal_commands.load_dll.loading", "dll_path"_a = shared_library_path);

	if (arg.val.model().is_running()) {
		arg.val.model().async_load_dll(shared_library_path);
	}
	else {
		if (!arg.val.model().load_dll(shared_library_path)) {
			arg.term.add_text(log_get_or_gen(arg, "terminal_commands.load_dll.loading_failed"));
		}
	}
}

void terminal_commands::load_map(argument_type &arg) {
	if (arg.command_line.size() < 2) {
		log_formatted_err(arg, "terminal_commands.load_map.usage", "arg0"_a = arg.command_line[0]);
		return;
	}
	arg.val.adapter().load_map(arg.command_line[1]);
}

void terminal_commands::update_world(argument_type &arg) {
	arg.val.model().bot_think();
	arg.val.adapter().m_cells_changed_since_last_update.clear();
	arg.val.model().world.update(arg.val.adapter());
}

void terminal_commands::run_model(argument_type &arg) {
	arg.val.model().run();
	arg.val.view();
}

void terminal_commands::stop_model(argument_type &arg) {
	arg.val.model().stop();
}

void terminal_commands::set(argument_type &arg) {
	if (arg.command_line.size() == 3) {
		auto it = arg.val.properties().find(arg.command_line[1]);
		if (it == arg.val.properties().end()) {
			log_formatted_err(arg, "terminal_commands.set.unknown_var", "var_name"_a = arg.command_line[1]);
			return;
		}

		utils::visitor property_visitor{[&](state::property::proxy<unsigned int> &p) {
			                                std::optional<unsigned int> value = utils::from_chars<unsigned int>(arg.command_line.back());
			                                if (value) {
				                                p.set(*value);
			                                }
			                                else {
				                                log_formatted_err(arg, "terminal_commands.set.need_uint",
				                                                  "value"_a = arg.command_line.back());
			                                }
		                                },
		                                [&](state::property::proxy<std::atomic_bool> &p) {
			                                p.set(as_bool(arg.command_line.back()));
		                                }};
		utils::visitor property_dispatcher{[&](const state::property::readonly_property & /* ignored*/) {
			                                   log_formatted_err(arg, "terminal_commands.set.is_read_only",
			                                                     "var_name"_a = arg.command_line[1]);
		                                   },
		                                   [&](state::property::settable_property p) {
			                                   std::visit(property_visitor, p);
		                                   }};

		std::visit(property_dispatcher, it->second.data());
	}
	else {
		log_formatted_err(arg, "terminal_commands.set.usage", "arg0"_a = arg.command_line.front());
	}
}

void terminal_commands::valueof(argument_type &arg) {
	if (arg.command_line.size() == 2) {
		auto it = arg.val.properties().find(arg.command_line.back());
		if (it == arg.val.properties().end()) {
			log_formatted_err(arg, "terminal_commands.get.unknown_var", "var_name"_a = arg.command_line[1]);
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
		log_formatted_err(arg, "terminal_commands.get.usage", "arg0"_a = arg.command_line.front());
	}
}

void terminal_commands::reconfigure(argument_type &arg) {
	if (arg.command_line.size() != 2) {
		log_formatted_err(arg, "terminal_commands.reload.usage", "arg0"_a = arg.command_line.front());
		return;
	}

	if (!arg.val.resources().reload(arg.command_line.back())) {
		log_formatted(arg, "terminal_commands.reload.fail", "file_path"_a = arg.command_line.back());
	}
	else {
		log_formatted(arg, "terminal_commands.reload.success", "file_path"_a = arg.command_line.back());
		arg.val.view().reload_sprites();
		arg.val.terminal().get_terminal_helper()->load_commands(arg.val.resources());
	}
}

void terminal_commands::fire_activator(argument_type &arg) {
	if (arg.command_line.size() != 2) {
		log_formatted_err(arg, "terminal_commands.fire_activator.usage", "arg0"_a = arg.command_line.front());
		return;
	}

	std::optional<unsigned int> val = utils::from_chars<unsigned int>(arg.command_line[1]);
	if (!val) {
		log_formatted_err(arg, "terminal_commands.fire_activator.need_uint", "value"_a = arg.command_line.back());
		return;
	}

	if (*val >= arg.val.model().world.activators.size()) {
			log_formatted_err(arg, "terminal_commands.fire_activator.too_high", "value"_a = arg.command_line.back(),
			                  "max_value"_a = arg.val.model().world.activators.size());
		return;
	}

	arg.val.model().world.fire_activator(arg.val.adapter(), *val);
}

void terminal_commands::fire_actionable(argument_type &arg) {
	if (arg.command_line.size() != 2) {
		log_formatted_err(arg, "terminal_commands.fire_activator.usage", "arg0"_a = arg.command_line.front());
		return;
	}

	std::optional<unsigned int> val = utils::from_chars<unsigned int>(arg.command_line[1]);
	if (!val) {
		log_formatted_err(arg, "terminal_commands.fire_activator.need_uint", "value"_a = arg.command_line.back());
		return;
	}

	if (*val >= arg.val.model().world.actionables.size()) {
		log_formatted_err(arg, "terminal_commands.fire_activator.too_high", "value"_a = arg.command_line.back(),
		                  "max_value"_a = arg.val.model().world.actionables.size());
		return;
	}
	arg.val.model().world.fire_actionable(arg.val.adapter(), *val);
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

	auto it = arg.val.properties().lower_bound(arg.command_line.back());
	while (it != arg.val.properties().end() && utils::starts_with(it->first, arg.command_line.back())) {
		ans.emplace_back(it++->first);
	}
	return ans;
}
