#ifndef NINJACLOWN_INTERACTION_HPP
#define NINJACLOWN_INTERACTION_HPP

namespace model {

enum class interaction_kind {
	LIGHT_MANUAL   = 1, // character or thrown item can interact
	HEAVY_MANUAL   = 2, // only a character can interact
	LIGHT_MIDAIR   = 3, // character or thrown item in the cell cause interaction
	HEAVY_MIDAIR   = 4, // only character in the cell cause interaction
	WALK_ON_GROUND = 5, // only non-floating character in the cell cause interaction
};

enum class interactable_kind {
	BUTTON,
	INDUCTION_LOOP,
	INFRARED_LASER,
};

struct interaction {
	interaction_kind kind;
	interactable_kind interactable;
	std::size_t interactable_handler;
};

} // namespace model

#endif //NINJACLOWN_INTERACTION_HPP
