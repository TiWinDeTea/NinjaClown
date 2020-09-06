#ifndef OS_WINDOWS

#include <model/collision.hpp>
#include <model/vec2.hpp>
#include <utils/universal_constants.hpp>

#include <catch2/catch.hpp>

bool operator==(const model::vec2 &a, const model::vec2 &b) noexcept {
	return a.x == Approx(b.x).margin(0.01) && a.y == Approx(b.y).margin(0.01);
}

SCENARIO("OBB OBB SAT collisions") {
	model::component::hitbox a{1.5f, 1.5f, 0.5f, 0.5f};

	model::obb a_box{a};

	a.rad = uni::math::pi<float>;
	model::obb rotated_a_box{a};

	GIVEN("Points A") {
		CHECK(a_box.tl == model::vec2{1.f, 2.f});
		CHECK(a_box.tr == model::vec2{2.f, 2.f});
		CHECK(a_box.bl == model::vec2{1.f, 1.f});
		CHECK(a_box.br == model::vec2{2.f, 1.f});

		CHECK(rotated_a_box.tl == model::vec2{2.f, 1.f});
		CHECK(rotated_a_box.tr == model::vec2{1.f, 1.f});
		CHECK(rotated_a_box.bl == model::vec2{2.f, 2.f});
		CHECK(rotated_a_box.br == model::vec2{1.f, 2.f});
	}

	model::component::hitbox b{4.f, 3.f, 0.42f, 0.42f};

	b.rad = uni::math::pi_4<float>;
	model::obb b_box{b};

	GIVEN("Points B") {
		CHECK(b_box.tl == model::vec2{3.4f, 3.f});
		CHECK(b_box.tr == model::vec2{4.f, 3.6f});
		CHECK(b_box.bl == model::vec2{4.f, 2.4f});
		CHECK(b_box.br == model::vec2{4.6f, 3.f});
	}

	GIVEN("Axises") {
		CHECK(a_box.tl.to(a_box.bl).atan2() == Approx(-uni::math::pi_2<float>).margin(0.01));
		CHECK(a_box.tl.to(a_box.tr).atan2() == Approx(0).margin(0.01));
		CHECK(b_box.tl.to(b_box.bl).atan2() == Approx(-uni::math::pi_4<float>).margin(0.01));
		CHECK(b_box.tl.to(b_box.tr).atan2() == Approx(uni::math::pi_4<float>).margin(0.01));
	}

	model::component::hitbox c{0.8f, 2.f, 0.31f, 0.31f};
	c.rad = uni::math::pi_4<float>;
	model::obb c_box{c};

	GIVEN("Collisions") {
		CHECK(!model::obb_obb_sat_test(a_box, b_box));
		CHECK(!model::obb_obb_sat_test(rotated_a_box, b_box));
		CHECK(model::obb_obb_sat_test(c_box, a_box));
		CHECK(model::obb_obb_sat_test(c_box, rotated_a_box));
	}
}

#endif
