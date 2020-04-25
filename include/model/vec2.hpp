#ifndef NINJACLOWN_MODEL_VECTOR_HPP
#define NINJACLOWN_MODEL_VECTOR_HPP

namespace model {

struct vec2 {
	float x;
	float y;

	[[nodiscard]] float dot(const vec2 &other) const;
	[[nodiscard]] vec2 unit() const;
	void unitify();
	[[nodiscard]] vec2 project_on(const vec2 &other) const;
	[[nodiscard]] vec2 proj_point(const vec2 &other) const;
	[[nodiscard]] float proj_coef(const vec2 &other) const;
	[[nodiscard]] vec2 normal() const;
	[[nodiscard]] vec2 to(const vec2 &other) const;
};

vec2 operator+(const vec2 &a, const vec2 &b);
vec2 operator-(const vec2 &a, const vec2 &b);
vec2 operator*(const vec2 &v, float scalar);
vec2 operator*(float scalar, const vec2 &v);

} // namespace model

#endif //NINJACLOWN_MODEL_VECTOR_HPP
