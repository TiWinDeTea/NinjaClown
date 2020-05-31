#ifndef NINJACLOWN_COMPONENTS_HPP
#define NINJACLOWN_COMPONENTS_HPP

#include <array>
#include <cmath>
#include <cstdint>
#include <ninja_clown/api.h>
#include <optional>
#include <variant>

#include "utils/universal_constants.hpp"
#include "vec2.hpp"

namespace model::component {

constexpr float default_move_speed     = 0.2f;
constexpr float default_rotation_speed = 0.2f;
constexpr float default_attack_range   = 1.0f;
constexpr float default_activate_range = 1.0f;

using decision = std::variant<ninja_api::nnj_movement_request, ninja_api::nnj_activate_request, ninja_api::nnj_attack_request,
                              ninja_api::nnj_throw_request>;

struct properties {
	float move_speed     = default_move_speed;
	float rotation_speed = default_rotation_speed;
	float attack_range   = default_attack_range;
	float activate_range = default_activate_range;
};

struct metadata {
	ninja_api::nnj_entity_kind kind = ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY;
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
	constexpr size_t max_entities = 10;
}

struct components {
	std::array<std::optional<component::health>, cst::max_entities> health;
	std::array<std::optional<component::hitbox>, cst::max_entities> hitbox;
	std::array<std::optional<component::decision>, cst::max_entities> decision;
	std::array<component::properties, cst::max_entities> properties;
	std::array<component::metadata, cst::max_entities> metadata;
};

} // namespace model

#endif //NINJACLOWN_COMPONENTS_HPP
