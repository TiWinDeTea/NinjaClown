#ifndef OS_WINDOWS

#include <model/collision.hpp>
#include <model/vec2.hpp>
#include <utils/universal_constants.hpp>

#include <catch2/catch.hpp>

// NOLINTBEGIN

namespace model {
	bool operator==(const vec2 &a, const vec2 &b) noexcept {
		return a.x == Approx(b.x).margin(0.01) && a.y == Approx(b.y).margin(0.01);
	}
}

SCENARIO("OBB OBB SAT collisions") {
	model::component::hitbox hitbox_a{1.5f, 1.5f, 0.5f, 0.5f};

	model::obb box_a{hitbox_a};

	hitbox_a.rad = uni::math::pi<float>;
	model::obb rotated_a{hitbox_a};

	GIVEN("Points A") {
		CHECK(box_a.tl == model::vec2{1.f, 2.f});
		CHECK(box_a.tr == model::vec2{2.f, 2.f});
		CHECK(box_a.bl == model::vec2{1.f, 1.f});
		CHECK(box_a.br == model::vec2{2.f, 1.f});

		CHECK(rotated_a.tl == model::vec2{2.f, 1.f});
		CHECK(rotated_a.tr == model::vec2{1.f, 1.f});
		CHECK(rotated_a.bl == model::vec2{2.f, 2.f});
		CHECK(rotated_a.br == model::vec2{1.f, 2.f});
	}

	model::component::hitbox hitbox_b{4.f, 3.f, 0.42f, 0.42f};

	hitbox_b.rad = uni::math::pi_4<float>;
	model::obb box_b{hitbox_b};

	GIVEN("Points B") {
		CHECK(box_b.tl == model::vec2{3.4f, 3.f});
		CHECK(box_b.tr == model::vec2{4.f, 3.6f});
		CHECK(box_b.bl == model::vec2{4.f, 2.4f});
		CHECK(box_b.br == model::vec2{4.6f, 3.f});
	}

	GIVEN("Axises") {
		CHECK(box_a.tl.to(box_a.bl).atan2() == Approx(-uni::math::pi_2<float>).margin(0.01));
		CHECK(box_a.tl.to(box_a.tr).atan2() == Approx(0).margin(0.01));
		CHECK(box_b.tl.to(box_b.bl).atan2() == Approx(-uni::math::pi_4<float>).margin(0.01));
		CHECK(box_b.tl.to(box_b.tr).atan2() == Approx(uni::math::pi_4<float>).margin(0.01));
	}

	model::component::hitbox hitbox_c{0.8f, 2.f, 0.31f, 0.31f};
	hitbox_c.rad = uni::math::pi_4<float>;
	model::obb box_c{hitbox_c};

	GIVEN("Collisions") {
		CHECK(!model::obb_obb_sat_test(box_a, box_b));
		CHECK(!model::obb_obb_sat_test(rotated_a, box_b));
		CHECK(model::obb_obb_sat_test(box_c, box_a));
		CHECK(model::obb_obb_sat_test(box_c, rotated_a));
	}
}

// NOLINTEND

#endif
