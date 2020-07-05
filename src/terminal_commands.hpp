#ifndef NINJACLOWN_TERMINAL_COMMANDS_HPP
#define NINJACLOWN_TERMINAL_COMMANDS_HPP

#include <initializer_list>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include <imterm/terminal.hpp>
#include <imterm/terminal_helpers.hpp>

namespace utils {
class resource_manager;
}
namespace spdlog {
class logger;
}
namespace state {
class holder;
}

class terminal_commands: public ImTerm::basic_spdlog_terminal_helper<terminal_commands, state::holder, misc::no_mutex> {
public:
	void load_commands(const utils::resource_manager &resources) noexcept;

	void set_terminal(term_t &term) noexcept;

private:
	// overloading because some messages may be logged before the terminal has been linked to this instance
	void sink_it_(const spdlog::details::log_msg &msg) override;

	// messages that were logged before a terminal was linked to this instance
	std::vector<ImTerm::message> m_pre_logged_messages{};

public:
	static std::vector<std::string> no_completion(argument_type &) {
		return {};
	}

	static void clear(argument_type &);
	static void echo(argument_type &);
	static void exit(argument_type &);
	static void help(argument_type &);
	static void quit(argument_type &);
	static void load_shared_library(argument_type &);
	static void load_map(argument_type &);
	static void update_world(argument_type &);
	static void run_model(argument_type &);
	static void stop_model(argument_type &);
	static void set(argument_type &);
	static void valueof(argument_type &);
	static void reconfigure(argument_type &);
	static void fire_activator(argument_type &);
	static void fire_actionable(argument_type &);

	static std::vector<std::string> autocomplete_path(argument_type &, const std::initializer_list<std::string_view> &extensions);
	static std::vector<std::string> autocomplete_variable(argument_type &);
};

#endif // NINJACLOWN_TERMINAL_COMMANDS_HPP
