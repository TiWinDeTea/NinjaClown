#include <cassert>

#include "facing_dir.hpp"
#include "utils/universal_constants.hpp"

// TODO: add NE/NW/SE/SW
view::facing_direction::type view::facing_direction::from_angle(float rad) {
    assert(rad >= -uni::math::pi<float> - 0.01);
    assert(rad <= uni::math::pi<float> + 0.01);

	if (rad >= 7 * uni::math::pi<float> / 8) {
		return W;
	}

	if (rad >= 5 * uni::math::pi<float> / 8) {
		return NW;
	}

	if (rad >= 3 * uni::math::pi<float> / 8) {
		return N;
	}

	if (rad >= uni::math::pi<float> / 8) {
		return NE;
	}

	if (rad >= -uni::math::pi<float> / 8) {
		return E;
	}

	if (rad >= -3 * uni::math::pi<float> / 8) {
		return SE;
	}

	if (rad >= -5 * uni::math::pi<float> / 8) {
		return S;
	}

	if (rad >= -7 * uni::math::pi<float> / 8) {
		return SW;
	}

	return W;
}
