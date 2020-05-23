#ifndef NINJACLOWN_MODEL_VECTOR_HPP
#define NINJACLOWN_MODEL_VECTOR_HPP

#include <ostream>

namespace model {

struct vec2 {
	float x;
	float y;

	vec2 &unitify();
	[[nodiscard]] float dot(const vec2 &other) const noexcept;
	[[nodiscard]] vec2 unit() const noexcept;
	[[nodiscard]] vec2 project_on(const vec2 &other) const noexcept;
	[[nodiscard]] vec2 proj_point(const vec2 &other) const noexcept;
	[[nodiscard]] float proj_coef(const vec2 &other) const noexcept;
	[[nodiscard]] vec2 normal() const noexcept;
	[[nodiscard]] float norm() const noexcept;
	[[nodiscard]] vec2 to(const vec2 &other) const noexcept;
	[[nodiscard]] float prod(const vec2 &other) const noexcept;
	[[nodiscard]] float atan2() const noexcept;
};

vec2 operator+(const vec2 &a, const vec2 &b);
vec2 operator-(const vec2 &a, const vec2 &b);
vec2 operator*(const vec2 &v, float scalar);
vec2 operator*(float scalar, const vec2 &v);

} // namespace model

std::ostream &operator<<(std::ostream &stream, const model::vec2 &vec);

#endif //NINJACLOWN_MODEL_VECTOR_HPP
