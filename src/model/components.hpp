#ifndef NINJACLOWN_COMPONENTS_HPP
#define NINJACLOWN_COMPONENTS_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <ninja_clown/api.h>
#include <optional>
#include <variant>

#include "model/types.hpp"
#include "model/vec2.hpp"
#include "utils/universal_constants.hpp"

namespace model::component {

constexpr float default_move_speed     = 0.2f;
constexpr float default_rotation_speed = 0.2f;
constexpr float default_attack_range   = 1.0f;
constexpr float default_activate_range = 1.0f;
constexpr tick_t default_attack_delay  = 5;
constexpr tick_t default_throw_delay   = 3;

struct movement {
	float rotation = 0.f; // radians
	float forward_diff = 0.f;
	float lateral_diff = 0.f;
};

using decision = std::variant<ninja_api::nnj_movement_request, ninja_api::nnj_activate_request, ninja_api::nnj_attack_request,
                              ninja_api::nnj_throw_request>;

using preparable_action = std::variant<ninja_api::nnj_activate_request, ninja_api::nnj_attack_request, ninja_api::nnj_throw_request>;

struct properties {
	float move_speed     = default_move_speed;
	float rotation_speed = default_rotation_speed;
	float attack_range   = default_attack_range;
	float activate_range = default_activate_range;
	tick_t attack_delay  = default_attack_delay;
	tick_t throw_delay   = default_throw_delay;
};

struct metadata {
	ninja_api::nnj_entity_kind kind = ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY;
};

struct state {
	utils::optional<preparable_action> preparing_action = {};
	tick_t ticks_before_ready                           = 0;
};

struct health {
	std::uint8_t points;
};

struct hitbox {
	vec2 center;
	vec2 half;
	float rad;

	hitbox(float center_x, float center_y, float half_width, float half_height) noexcept
	    : center{center_x, center_y}
	    , half{half_width, half_height}
	    , rad{0.f} { }

	[[nodiscard]] vec2 top_left() const {
		constexpr float top_left_angle = 3 * uni::math::pi_4<float>;
		float hypot                    = std::hypot(half.x, half.y);
		float angle                    = top_left_angle + rad;
		return vec2{hypot * std::cos(angle) + center.x, hypot * std::sin(angle) + center.y};
	}

	[[nodiscard]] vec2 top_right() const {
		constexpr float top_right_angle = uni::math::pi_4<float>;
		float hypot                     = std::hypot(half.x, half.y);
		float angle                     = top_right_angle + rad;
		return vec2{hypot * std::cos(angle) + center.x, hypot * std::sin(angle) + center.y};
	}

	[[nodiscard]] vec2 bottom_left() const {
		constexpr float bottom_left_angle = -3 * uni::math::pi_4<float>;
		float hypot                       = std::hypot(half.x, half.y);
		float angle                       = bottom_left_angle + rad;
		return vec2{hypot * std::cos(angle) + center.x, hypot * std::sin(angle) + center.y};
	}

	[[nodiscard]] vec2 bottom_right() const {
		constexpr float bottom_right_angle = -uni::math::pi_4<float>;
		float hypot                        = std::hypot(half.x, half.y);
		float angle                        = bottom_right_angle + rad;
		return vec2{hypot * std::cos(angle) + center.x, hypot * std::sin(angle) + center.y};
	}

	[[nodiscard]] float half_width() const {
		return half.x;
	}

	[[nodiscard]] float half_height() const {
		return half.y;
	}

	[[nodiscard]] float width() const {
		return half.x * 2;
	}

	[[nodiscard]] float height() const {
		return half.y * 2;
	}
};
} // namespace model::component

namespace model {

namespace cst {
	// NOTE: it’s crucial for max_entities to not be too big because we are using a very naive algorithm for collisions
	constexpr size_t max_entities = 32;
}

struct components {
	std::array<std::optional<component::health>, cst::max_entities> health;
	std::array<std::optional<component::hitbox>, cst::max_entities> hitbox;
	std::array<std::optional<component::decision>, cst::max_entities> decision;
	std::array<std::optional<component::movement>, cst::max_entities> movement;
	std::array<component::properties, cst::max_entities> properties;
	std::array<component::metadata, cst::max_entities> metadata;
	std::array<component::state, cst::max_entities> state;
};

} // namespace model

#endif //NINJACLOWN_COMPONENTS_HPP
