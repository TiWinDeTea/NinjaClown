#ifndef NINJACLOWN_MODEL_COLLISION_HPP
#define NINJACLOWN_MODEL_COLLISION_HPP

#include <utility>

#include "components.hpp"

namespace model {

struct bounding_box {
	using min = float;
	using max = float;

	bounding_box(float top_left_x, float top_left_y, float width, float height) noexcept
	    : tl{top_left_x, top_left_y}
	    , bl{top_left_x, top_left_y + height}
	    , tr{top_left_x + width, top_left_y}
	    , br{top_left_x + width, top_left_y + height} { }

	explicit bounding_box(const component::hitbox &box) noexcept
	    : tl{box.top_left()}
	    , bl{box.bottom_left()}
	    , tr{box.top_right()}
	    , br{box.bottom_right()} { }

	[[nodiscard]] std::pair<min, max> compute_proj_coefs(const vec2 &axis) const;

	vec2 tl;
	vec2 bl;
	vec2 tr;
	vec2 br;
};

bool obb_obb_sat_test(const bounding_box &a, const bounding_box &b);

struct bounding_circle {
	explicit bounding_circle(const component::hitbox &box) noexcept
	    : center{box.center}
	    , radius{std::min(box.half_height(), box.half_height())} { }

	vec2 center;
	float radius;
};

bool circle_obb_test(const bounding_circle &a, const bounding_box &b);

} // namespace model

#endif //NINJACLOWN_MODEL_COLLISION_HPP
