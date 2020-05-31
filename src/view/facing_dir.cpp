#include <cassert>

#include "view/facing_dir.hpp"
#include "utils/universal_constants.hpp"

view::facing_direction::type view::facing_direction::from_angle(float rad) {
    assert(rad >= -uni::math::pi<float>);
    assert(rad <= uni::math::pi<float>);

	if (rad <= 0) {
		if (rad <= -3.f * uni::math::pi<float> / 4.f) {
			return W;
		}
		if (rad <= -uni::math::pi<float> / 4.f) {
			return S;
		}
		return E;
	}
	if (rad >= 3.f * uni::math::pi<float> / 4.f) {
		return W;
	}
	if (rad >= uni::math::pi<float> / 4.f) {
		return N;
	}
	return E;
}
