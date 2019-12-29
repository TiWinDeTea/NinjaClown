#ifndef NINJACLOWN_INTERACTABLE_HPP
#define NINJACLOWN_INTERACTABLE_HPP

namespace model {
class world; // forward declaration

enum class interaction_kind {
	LIGHT_MANUAL, // character or thrown item can interact
	HEAVY_MANUAL, // only a character can interact
	LIGHT_MIDAIR, // character or thrown item in the cell cause interaction
	HEAVY_MIDAIR, // only character in the cell cause interaction
	GROUND, // only non-floating character in the cell cause interaction
};

class interactable {
public:
	virtual std::unique_ptr<interactable> clone() = 0;
	virtual interaction_kind kind()               = 0;
	virtual void interact(world &world)           = 0;
};
} // namespace model

#endif //NINJACLOWN_INTERACTABLE_HPP
