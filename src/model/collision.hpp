#ifndef NINJACLOWN_MODEL_COLLISION_HPP
#define NINJACLOWN_MODEL_COLLISION_HPP

#include <utility>

#include "model/cell.hpp"
#include "model/components.hpp"
#include "model/grid_point.hpp"

namespace model {

struct bounding_circle {
    explicit bounding_circle(const component::hitbox &box) noexcept
      : center{box.center}
      , radius{std::min(box.half_height(), box.half_height())} { }

    vec2 center;
    float radius;
};

struct obb {
	using min = float;
	using max = float;

	obb(float top_left_x, float top_left_y, float width, float height) noexcept
	    : tl{top_left_x, top_left_y}
	    , bl{top_left_x, top_left_y + height}
	    , tr{top_left_x + width, top_left_y}
	    , br{top_left_x + width, top_left_y + height} { }

	explicit obb(const component::hitbox &box) noexcept
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

struct aabb {
	aabb(float top_left_x, float top_left_y, float width, float height) noexcept
	    : top_left{top_left_x, top_left_y}
	    , bottom_right{top_left_x + width, top_left_y + height} { }

	explicit aabb(grid_point cell_pos) noexcept
	    : aabb(static_cast<float>(cell_pos.x), static_cast<float>(cell_pos.y), cst::cell_width, cst::cell_height) { }

	[[nodiscard]] vec2 center() const noexcept {
		return {(top_left.x + bottom_right.x) / 2, (top_left.y + bottom_right.y) / 2};
	}

	vec2 top_left;
	vec2 bottom_right;
};

bool obb_obb_sat_test(const obb &a, const obb &b);

bool circle_aabb_test(const bounding_circle &circle, const aabb &box);

bool point_aabb_test(const vec2 &point, const aabb &box);

} // namespace model

#endif //NINJACLOWN_MODEL_COLLISION_HPP
