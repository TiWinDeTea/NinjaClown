#ifndef NINJACLOWN_COMPONENTS_HPP
#define NINJACLOWN_COMPONENTS_HPP

#include <optional>

namespace model::component {

constexpr float DEFAULT_MOVE_SPEED = 0.2;
constexpr float DEFAULT_ROTATION_SPEED = 0.2;

struct properties {
	float move_speed = DEFAULT_MOVE_SPEED;
	float rotation_speed = DEFAULT_ROTATION_SPEED;
};

struct health {
	std::uint8_t points;
};

struct hitbox {
	float x;
	float y;
	float width;
	float height;

	float right_x() const {
		return x + width;
	}
	float bottom_y() const {
		return y + height;
	}
	float center_x() const {
		return x + (width / 2);
	}
	float center_y() const {
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

constexpr const char *to_string(decision d) {
#define COMPONENTS_DECISION_CASE(x)                                                                                                        \
	case decision::x:                                                                                                                      \
		return #x
	// TODO: use resource_manager::to_string(decision);
	switch (d) {
		COMPONENTS_DECISION_CASE(TURN_LEFT);
		COMPONENTS_DECISION_CASE(TURN_RIGHT);
		COMPONENTS_DECISION_CASE(MOVE_FORWARD);
		COMPONENTS_DECISION_CASE(MOVE_BACKWARD);
		COMPONENTS_DECISION_CASE(ACTIVATE_BUTTON);
		default:
			return "UNKNOWN";
	}
#undef COMPONENTS_DECISION_CASE
}

} // namespace model::component

namespace model {

constexpr size_t MAX_ENTITIES = 10;

struct components {
	std::optional<component::health> health[MAX_ENTITIES];
	std::optional<component::hitbox> hitbox[MAX_ENTITIES];
	std::optional<component::angle> angle[MAX_ENTITIES];
	std::optional<component::decision> decision[MAX_ENTITIES];
	component::properties properties[MAX_ENTITIES];
};

} // namespace model

#endif //NINJACLOWN_COMPONENTS_HPP
