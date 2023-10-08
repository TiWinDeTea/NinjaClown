#include "terminal_commands.hpp"

#include <array>
#include <filesystem>

#include <model/event.hpp>
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
#include "view/game_viewer.hpp"
#include "view/view.hpp"

using fmt::literals::operator""_a;

namespace {
/**
 * Replaces all '\' with '/'
 */
std::string escape(std::string str) {
	std::string::size_type pos = str.find('\\');
	while (pos != std::string::npos) {
		str[pos] = '/';
		pos      = str.find('\\', pos);
	}
	return std::move(str);
}

/**
 * Separates a path into director (.first) and file (.second)
 */
std::pair<std::string_view, std::string_view> split_path(std::string_view path) {
	std::string_view file;
	std::string_view directory;

	if (auto last_dir_sep = path.find_last_of("/\\"); last_dir_sep != std::string_view::npos) {
		file        = path.substr(last_dir_sep + 1);
		directory   = path.substr(0, last_dir_sep);
		if (directory.empty()) {
			directory = "/";
		}
	}
	else {
		directory   = ".";
		file        = path;
	}

	return {directory, file};
}

/**
 * Fetches for the localized logging text corresponding to the key
 * @param arg argument_type, for access to resources
 * @param key key associated to the logging text
 */
std::string_view log_get(const terminal_commands::argument_type &arg, std::string_view key) {
	return arg.val.resources().log_for(key);
}

/**
 * Logs a localized message to the terminal [colorless]
 * @param arg argument_type, for access to resources
 * @param key key associated to the logging text
 * @param args arguments of the logging text, forwarded to fmt
 *
 * @see log_formatted_err
 */
template <typename... Args>
void log_formatted(terminal_commands::argument_type &arg, std::string_view key, Args &&... args) {
	const std::string_view fmt = log_get(arg, key);
	try {
		arg.term.add_formatted(fmt.data(), std::forward<Args>(args)...);
	}
	catch (const fmt::format_error &error) {
		spdlog::error(R"("{}" while formatting string "{}")", error.what(), fmt);
	}
}


/**
 * Logs a localized message as an error to the terminal (severity : warn) [colorless]
 * @param arg argument_type, for access to resources
 * @param key key associated to the logging text
 * @param args arguments of the logging text, forwarded to fmt
 *
 * @see log_formatted
 */
template <typename... Args>
void log_formatted_err(terminal_commands::argument_type &arg, std::string_view key, Args &&... args) {
	const std::string_view fmt = log_get(arg, key);
	try {
		arg.term.add_formatted_err(fmt.data(), std::forward<Args>(args)...);
	}
	catch (const fmt::format_error &error) {
		spdlog::error(R"("{}" while formatting string "{}")", error.what(), fmt);
	}
}

/**
 * @param arg path prefix
 * @return a list of files ending by ".so" or ".dll" corresponding to prefix, and a list of folders corresponding to prefix
 */
std::vector<std::string> autocomplete_library_path(terminal_commands::argument_type &arg) {
	return terminal_commands::autocomplete_path(arg, {".so", ".dll"});
}

/**
 * @param arg path prefix
 * @return a list of files ending by ".map" corresponding to prefix, and a list of folders corresponding to prefix
 */
std::vector<std::string> autocomplete_map_path(terminal_commands::argument_type &arg) {
	return terminal_commands::autocomplete_path(arg, {".map"});
}

/**
 * @param arg path prefix
 * @return a list of files ending by ".toml" corresponding to prefix, and a list of folders corresponding to prefix
 */
std::vector<std::string> autocomplete_config(terminal_commands::argument_type &arg) {
	return terminal_commands::autocomplete_path(arg, {".toml"});
}

//! used to store the list of available commands
struct cmd {
	command_id cmd; //! used to access localized resources
	void (*cmd_ptr)(terminal_commands::argument_type &); //! pointer to the function that will execute the command
	std::vector<std::string> (*complete_ptr)(terminal_commands::argument_type &); //! pointer to the autocompletion function for this command
};

/**
 * List of all the commands
 */
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

/**
 * Converts a string_view to a boolean. Converts to true if numeric and != 0, or if it compares equal to "true".
 */
bool as_bool(std::string_view str) {
	std::optional<int> int_val = utils::from_chars<int>(str);
	if (int_val) {
		return *int_val != 0;
	}
	return str == "true";
}
} // namespace

void terminal_commands::load_commands(const utils::resource_manager &resources) noexcept {
	cmd_list_.clear(); // add_command_ will fill this list

	for (const auto &cmd : local_command_list) {
		auto maybe_command = resources.text_for(cmd.cmd);
		if (!maybe_command) {
			utils::log::warn(resources, "terminal_commands.command_missing_name", "cmd_id"_a = static_cast<int>(cmd.cmd));
			continue;
		}

		auto [name, description] = *maybe_command;

		command_type command;
		command.call        = cmd.cmd_ptr;
		command.complete    = cmd.complete_ptr;
		command.description = description;
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
		m_pre_logged_messages.emplace_back(std::move(imsg)); // FIXME data race with terminal_commands::set_terminal ?
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
	for (auto it = std::next(arg.command_line.begin(), 1); it != arg.command_line.end(); ++it) {
		str += *it;
		str += ' ';
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
		commands.emplace_back(*opt);
	}

	const unsigned int element_max_name_size = misc::max_size(commands.begin(), commands.end(), [](const auto &str) {
		return str.first.size();
	});

	arg.term.add_text(log_get(arg, "terminal_commands.general.help").data());
	for (const auto &cmd : commands) {
		constexpr unsigned int raw_chars_count = 10;

		std::string str;
		str.reserve(raw_chars_count + element_max_name_size + cmd.second.size());
		str = "      ";
		str.append( element_max_name_size - cmd.first.size(), ' ');
		str += cmd.first;
		str += " | ";
		str += cmd.second;
		arg.term.add_text(str);

	}
	arg.term.add_text("");
}

void terminal_commands::quit(argument_type &arg) {
	arg.val.view().stop();
	// TODO fermer le thread logique ici ? c.f. state_holder::run, state_holder::wait
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
			arg.term.add_text(log_get(arg, "terminal_commands.load_dll.loading_failed").data());
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

	if (!arg.val.resources().reload(/*arg.command_line.back()*/)) { // FIXME : path to config is ignored
		log_formatted(arg, "terminal_commands.reload.fail", "file_path"_a = arg.command_line.back());
	}
	else {
		log_formatted(arg, "terminal_commands.reload.success", "file_path"_a = arg.command_line.back());
		arg.val.view().game().reload_sprites();
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
		if (arg.val.model().world.activators.empty()) {
			log_formatted_err(arg, "terminal_commands.fire_activator.none");
		}
		else {
			log_formatted_err(arg, "terminal_commands.fire_activator.too_high", "value"_a = arg.command_line.back(),
			                  "max_value"_a = arg.val.model().world.activators.size() - 1);
		}
		return;
	}

	arg.val.model().world.fire_activator(arg.val.adapter(), *val, model::event_reason::NONE);
}

void terminal_commands::fire_actionable(argument_type &arg) {
	if (arg.command_line.size() != 2) {
		log_formatted_err(arg, "terminal_commands.fire_actionable.usage", "arg0"_a = arg.command_line.front());
		return;
	}

	std::optional<unsigned int> val = utils::from_chars<unsigned int>(arg.command_line[1]);
	if (!val) {
		log_formatted_err(arg, "terminal_commands.fire_actionable.need_uint", "value"_a = arg.command_line.back());
		return;
	}

	if (*val >= arg.val.model().world.actionables.size()) {
		if (arg.val.model().world.actionables.empty()) {
			log_formatted_err(arg, "terminal_commands.fire_actionable.none");
		}
		else {
			log_formatted_err(arg, "terminal_commands.fire_actionable.too_high", "value"_a = arg.command_line.back(),
			                  "max_value"_a = arg.val.model().world.actionables.size() - 1);
		}
		return;
	}
	arg.val.model().world.fire_actionable(arg.val.adapter(), *val);
}

std::vector<std::string> terminal_commands::autocomplete_path(argument_type &arg,
                                                              const std::initializer_list<std::string_view> &extensions) {
	std::vector<std::string> paths;

	if (arg.command_line.size() != 2) {
		return paths;
	}

	const std::string_view current = arg.command_line[1];
	auto [directory, file_prefix] = split_path(current);

	std::error_code ec;
	const std::filesystem::directory_iterator d_it(directory, ec);
	if (ec) {
		return paths;
	}

	unsigned int simple_file_count{0};
	for (const auto &entry : d_it) {
		if (!utils::starts_with(entry.path().filename().string(), file_prefix)) {
			continue;
		}

		if (entry.is_directory()) {
			paths.emplace_back(escape(entry.path().string() + "/"));
			continue;
		}

		const auto &ext = entry.path().extension();
		if (utils::contains(extensions, ext)) {
			paths.emplace_back(escape(entry.path().string()));
			std::swap(paths[simple_file_count++], paths.back());
			break;
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
