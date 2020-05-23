#include "model/collision.hpp"

#include <algorithm> // min_element, max_element

bool model::obb_obb_sat_test(const bounding_box &a, const bounding_box &b) {
	std::array<model::vec2, 4> axises = {a.tl.to(a.bl), a.tl.to(a.tr), b.tl.to(b.bl), b.tl.to(b.tr)};

	for (const model::vec2 &axis : axises) {
		auto [a_min_coef, a_max_coef] = a.compute_proj_coefs(axis);
		auto [b_min_coef, b_max_coef] = b.compute_proj_coefs(axis);

		if (a_min_coef > b_max_coef || a_max_coef < b_min_coef) {
			return false;
		}
	}

	return true;
}

std::pair<model::bounding_box::min, model::bounding_box::max> model::bounding_box::compute_proj_coefs(const model::vec2 &axis) const {
	std::array<float, 4> proj_coefs = {tl.proj_coef(axis), bl.proj_coef(axis), tr.proj_coef(axis), br.proj_coef(axis)};
	float min_coef                  = *std::min_element(std::begin(proj_coefs), std::end(proj_coefs));
	float max_coef                  = *std::max_element(std::begin(proj_coefs), std::end(proj_coefs));
	return {min_coef, max_coef};
}

bool model::circle_aabb_test(const bounding_circle &circle, const aabb &box) {
	if (point_aabb_test(circle.center, box)) {
		return true;
	}

	vec2 radius = circle.center.to(box.center()).unitify() * circle.radius;
	vec2 point  = circle.center + radius;

	return point_aabb_test(point, box);
}

bool model::point_aabb_test(const vec2 &point, const aabb &box) {
	return point.x > box.top_left.x && point.x < box.bottom_right.x && point.y > box.top_left.y && point.y < box.bottom_right.y;
}
