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
	/**
	 * Loads commands into ImTerm, based on static variable local_command_list
	 * @param resources Used for localized logging
	 */
	void load_commands(const utils::resource_manager &resources) noexcept;

	/**
	 * Automatically called by ImTerm when setting up.
	 * Logs messages that were spdlog'ed before the terminal was available.
	 */
	[[maybe_unused]] void set_terminal(term_t &term) noexcept;

private:
	/**
	 * Automatically called by spdlog when logs are sent in.
	 * Logs messages to the terminal or store them for later logging if the terminal wasn’t spawned yet.
	 */
	void sink_it_(const spdlog::details::log_msg &msg) override;

	// messages that were logged before a terminal was linked to this instance
	std::vector<ImTerm::message> m_pre_logged_messages{};

public:
	static std::vector<std::string> no_completion(argument_type &) {
		return {};
	}

	/**
	 * Clears the terminal from any messages (including logs)
	 */
	static void clear(argument_type &);

	/**
	 * Prints a message to the terminal (colorless)
	 */
	static void echo(argument_type &);

	/**
	 * Quits the terminal
	 */
	static void exit(argument_type &);

	/**
	 * Prints localized info about all available commands
	 */
	static void help(argument_type &);

	/**
	 * Quits the program
	 */
	static void quit(argument_type &);

	/**
	 * Loads a library file
	 */
	static void load_shared_library(argument_type &);

	/**
	 * Attemps to load a map
	 */
	static void load_map(argument_type &);

	/**
	 * Advances the game state by one tick
	 */
	static void update_world(argument_type &);

	/**
	 * Starts the simulation
	 */
	static void run_model(argument_type &);

	/**
	 * Pauses the simulation
	 */
	static void stop_model(argument_type &);

	/**
	 * Sets the value of a property
	 */
	static void set(argument_type &);

	/**
	 * Prints the value of a property
	 */
	static void valueof(argument_type &);

	/**
	 * Loads a config, given its file path
	 */
	static void reconfigure(argument_type &);

	/**
	 * Activates an activator (eg: flips a button)
	 */
	static void fire_activator(argument_type &);

	/**
	 * Activates an actionable (eg: a door)
	 */
	static void fire_actionable(argument_type &);

	/**
	 * Completes a paths with files and folders corresponding to the prefix, and matching an extension.
	 * Files are stored before folders.
	 */
	static std::vector<std::string> autocomplete_path(argument_type &, const std::initializer_list<std::string_view> &extensions);

	/**
	 * Completes a variable name.
	 */
	static std::vector<std::string> autocomplete_variable(argument_type &);
};

#endif // NINJACLOWN_TERMINAL_COMMANDS_HPP
