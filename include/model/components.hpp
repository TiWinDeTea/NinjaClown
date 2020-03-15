#ifndef NINJACLOWN_COMPONENTS_HPP
#define NINJACLOWN_COMPONENTS_HPP

#include <cmath>
#include <optional>

#include "vec2.hpp"

namespace model::component {

constexpr float default_move_speed     = 0.2f;
constexpr float default_rotation_speed = 0.2f;

struct properties {
	float move_speed     = default_move_speed;
	float rotation_speed = default_rotation_speed;
};

struct health {
	std::uint8_t points;
};

struct hitbox {
	vec2 center;
	vec2 half;
	float rad;

	hitbox(float x, float y, float half_width, float half_height) noexcept
	    : center{x, y}
	    , half{half_width, half_height}
	    , rad{0.f} {}

	[[nodiscard]] vec2 top_left() const {
		constexpr float top_left_angle = 3 * M_PI_4;
		float hypot                    = std::hypot(half.x, half.y);
		float angle                    = top_left_angle + rad;
		return vec2{half.x * std::cos(angle) * hypot + center.x, half.y * std::sin(angle) * hypot + center.y};
	}

	[[nodiscard]] vec2 top_right() const {
		constexpr float top_right_angle = M_PI_4;
		float hypot                     = std::hypot(half.x, half.y);
		float angle                     = top_right_angle + rad;
		return vec2{half.x * std::cos(angle) * hypot + center.x, half.y * std::sin(angle) * hypot + center.y};
	}

	[[nodiscard]] vec2 bottom_left() const {
		constexpr float bottom_left_angle = -3 * M_PI_4;
		float hypot                       = std::hypot(half.x, half.y);
		float angle                       = bottom_left_angle + rad;
		return vec2{half.x * std::cos(angle) * hypot + center.x, half.y * std::sin(angle) * hypot + center.y};
	}

	[[nodiscard]] vec2 bottom_right() const {
		constexpr float bottom_right_angle = -M_PI_4;
		float hypot                        = std::hypot(half.x, half.y);
		float angle                        = bottom_right_angle + rad;
		return vec2{half.x * std::cos(angle) * hypot + center.x, half.y * std::sin(angle) * hypot + center.y};
	}

	float half_width() const {
		return half.x;
	}

	float half_height() const {
		return half.y;
	}

	float width() const {
		return half.x * 2;
	}

	float height() const {
		return half.y * 2;
	}
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

constexpr size_t max_entities = 10;

struct components {
	std::optional<component::health> health[max_entities];
	std::optional<component::hitbox> hitbox[max_entities];
	std::optional<component::decision> decision[max_entities];
	component::properties properties[max_entities];
};

} // namespace model

#endif //NINJACLOWN_COMPONENTS_HPP
