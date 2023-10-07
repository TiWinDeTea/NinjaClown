#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "state_holder.hpp"

int actual_main(std::vector<std::string> &args);

#ifdef USE_WINMAIN
#include <shellapi.h>
#include <tchar.h>
#include <windows.h>

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
	while ((argc--) != 0) {
		args.emplace_back(*argv++);
	}
	return actual_main(args);
}
#endif

int actual_main([[maybe_unused]] std::vector<std::string> &args) {
	spdlog::default_logger()->set_level(spdlog::level::trace);

	state::holder game{"resources/autorun.ncs"};
	game.run();
	game.wait();

	return 0;

	// todo : ajouter un logger à spdlog qui fait des popups pour les erreurs
}
