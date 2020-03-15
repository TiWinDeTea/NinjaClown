#ifndef NINJACLOWN_MODEL_COLLISION_HPP
#define NINJACLOWN_MODEL_COLLISION_HPP

#include <utility>

#include "components.hpp"

namespace model {

struct bounding_box {
	using min = float;
	using max = float;

	explicit bounding_box(const model::component::hitbox &box) noexcept
	    : tl{box.top_left()}
	    , bl{box.bottom_left()}
	    , tr{box.top_right()}
	    , br{box.bottom_right()} {}

	std::pair<min, max> compute_proj_coefs(const model::vec2 &axis) const;

	model::vec2 tl;
	model::vec2 bl;
	model::vec2 tr;
	model::vec2 br;
};

bool obb_obb_sat_test(const bounding_box &a, const bounding_box &b);

} // namespace model

#endif //NINJACLOWN_MODEL_COLLISION_HPP
