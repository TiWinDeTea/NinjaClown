#ifndef NINJACLOWN_MODEL_VECTOR_HPP
#define NINJACLOWN_MODEL_VECTOR_HPP

namespace model {

struct vec2 {
	float x;
	float y;

	float dot(const vec2 &other) const;
	vec2 unit() const;
	void unitify();
	vec2 project_on(const vec2 &other) const;
	vec2 proj_point(const vec2 &other) const;
	float proj_coef(const vec2 &other) const;
	vec2 normal() const;
	vec2 to(const vec2 &other) const;
};

model::vec2 operator+(const model::vec2 &a, const model::vec2 &b);
model::vec2 operator-(const model::vec2 &a, const model::vec2 &b);
model::vec2 operator*(const model::vec2 &v, float scalar);
model::vec2 operator*(float scalar, const model::vec2 &v);

} // namespace model

#endif //NINJACLOWN_MODEL_VECTOR_HPP
