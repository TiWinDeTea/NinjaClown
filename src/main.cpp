#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "program_state.hpp"
#include "utils/scope_guards.hpp"
#include "view/viewer.hpp"

int actual_main(std::vector<std::string> &);

#ifdef USE_WINMAIN
#	include <shellapi.h>
#	include <tchar.h>
#	include <windows.h>
#	include <view/viewer.hpp>

INT WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, INT) {

	int argc;
	LPWSTR *lpArgv = CommandLineToArgvW(GetCommandLineW(), &argc);

	std::vector<std::string> args;
	args.reserve(argc);
	for (int i = 0; i < argc; ++i) {
		size_t size = wcslen(lpArgv[i]) + 1;
		std::string arg;
		arg.resize(size);
		wcstombs_s(nullptr, arg.data(), arg.size(), lpArgv[i], size);
		args.emplace_back(std::move(arg));
	}
	LocalFree(lpArgv);
	return actual_main(args);
}
#else
int main(int argc, char *argv[]) {
	std::vector<std::string> args;
	args.reserve(argc);
	while (argc--) {
		args.emplace_back(*argv++);
	}
	return actual_main(args);
}
#endif

#include <array>
#include <tuple>

int actual_main([[maybe_unused]] std::vector<std::string> &args) {
    // FIXME
    // Ce code a besoin d’amour

	union {
		alignas(program_state) char data[sizeof(program_state)];
	} prog_union;
	program_state &program = *reinterpret_cast<program_state *>(prog_union.data);
	program_state::global  = &program;

	new (&program.resource_manager) utils::resource_manager;
    if (!program.resource_manager.load_config("resources/config.toml")) {
        spdlog::critical("Failed to load resources.");
    }

	new (&program.terminal) ImTerm::terminal<terminal_commands>{program_state::s_empty, "terminal"};
	new (&program.viewer) view::viewer;
	new (&program.world) model::world;
	new (&program.bot_dll) bot::bot_dll;
	new (&program.adapter) adapter::adapter;
	new (&program.close_request) bool{false};
	new (&program.term_on_display) bool{true};
	ON_SCOPE_EXIT {
		program.~program_state();
	};

	spdlog::default_logger()->sinks().push_back(program.terminal.get_terminal_helper());
	spdlog::default_logger()->set_level(spdlog::level::trace);
	spdlog::info("~ Ninja. Clown. ~");

	program.viewer.run();
	program.viewer.wait();

	return 0;
}
