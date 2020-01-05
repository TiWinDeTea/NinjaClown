#ifndef NINJACLOWN_COMPONENTS_HPP
#define NINJACLOWN_COMPONENTS_HPP

#include <optional>

namespace model::component {

struct health {
	std::uint8_t points;
};

struct hitbox {
	float x;
	float y;
	float width;
	float height;

	float right_x() const
	{
		return x + width;
	}
	float bottom_y() const
	{
		return y + height;
	}
	float center_x() const
	{
		return x + (width / 2);
	}
	float center_y() const
	{
		return y + (height / 2);
	}
};

struct angle {
	float rad;
};

enum class decision {
	TURN_LEFT,
	TURN_RIGHT,
	MOVE_FORWARD,
	MOVE_BACKWARD,
	ACTIVATE_BUTTON,
};

} // namespace model::component

namespace model {

constexpr size_t MAX_ENTITIES = 10;

struct components {
	std::optional<component::health> health[MAX_ENTITIES];
	std::optional<component::hitbox> hitbox[MAX_ENTITIES];
	std::optional<component::angle> angle[MAX_ENTITIES];
	std::optional<component::decision> decision[MAX_ENTITIES];
};

} // namespace model

#endif //NINJACLOWN_COMPONENTS_HPP
