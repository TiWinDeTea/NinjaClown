
#include <imterm/terminal.hpp>

#include <spdlog/spdlog.h>

#include "program_state.hpp"
#include "terminal_commands.hpp"

#include "utils/optional.hpp"

int actual_main(std::vector<std::string> &);

#ifdef USE_WINMAIN
#	include <shellapi.h>
#	include <tchar.h>
#	include <windows.h>
#	include <view/viewer.hpp>

INT WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, INT)
{

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
int main(int argc, char *argv[])
{
	std::vector<std::string> args;
	args.reserve(argc);
	while (argc--) {
		args.emplace_back(*argv++);
	}
	return actual_main(args);
}
#endif

int actual_main([[maybe_unused]] std::vector<std::string> &args)
{
    program_state program;
    program_state::global = &program;
    program_state::global->viewer.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    program_state::global->viewer.wait();
    return 0;
}