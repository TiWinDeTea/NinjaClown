#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "state_holder.hpp"

int main() {
	spdlog::default_logger()->set_level(spdlog::level::trace);
	// TODO: ajouter un logger à spdlog qui fait des popups pour les erreurs

	state::holder game{"resources/autorun.ncs"};
	game.run();
	game.wait();

	return 0;
}
