#ifndef NINJACLOWN_INTERACTION_HPP
#define NINJACLOWN_INTERACTION_HPP

namespace model {
enum class interaction_kind {
	LIGHT_MANUAL, // character or thrown item can interact
	HEAVY_MANUAL, // only a character can interact
	LIGHT_MIDAIR, // character or thrown item in the cell cause interaction
	HEAVY_MIDAIR, // only character in the cell cause interaction
	GROUND, // only non-floating character in the cell cause interaction
};

enum class interactable_kind {
	BUTTON,
};

struct interaction {
	interaction_kind kind;
	interactable_kind interactable;
	size_t interactable_handler;
};
} // namespace model

#endif //NINJACLOWN_INTERACTION_HPP
