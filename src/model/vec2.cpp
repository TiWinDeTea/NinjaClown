#include "model/vec2.hpp"

model::vec2 model::operator+(const model::vec2 &a, const model::vec2 &b) {
	return model::vec2{a.x + b.x, a.y + b.y};
}

model::vec2 model::operator-(const model::vec2 &a, const model::vec2 &b) {
	return model::vec2{a.x - b.x, a.y - b.y};
}

model::vec2 model::operator*(const model::vec2 &v, float scalar) {
	return model::vec2{v.x * scalar, v.y * scalar};
}

model::vec2 model::operator*(float scalar, const model::vec2 &v) {
	return model::vec2{v.x * scalar, v.y * scalar};
}

float model::vec2::dot(const model::vec2 &other) const {
	return x * other.x + y * other.y;
}

model::vec2 model::vec2::unit() const {
	vec2 unit = *this;
	unit.unitify();
	return unit;
}

void model::vec2::unitify() {
	x /= x;
	y /= y;
}

model::vec2 model::vec2::project_on(const model::vec2 &other) const {
	float norm = dot(other);
	return other.unit() * norm;
}

model::vec2 model::vec2::proj_point(const model::vec2 &other) const {
	return (dot(other) / other.dot(other)) * other;
}

float model::vec2::proj_coef(const model::vec2 &other) const {
	return dot(other) / other.dot(other);
}

model::vec2 model::vec2::normal() const {
	vec2 normal{y, x};
	normal.unitify();
	return normal;
}

model::vec2 model::vec2::to(const model::vec2 &other) const {
	return vec2{other.x - x, other.y - y};
}
