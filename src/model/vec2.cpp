#include <cmath>

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

float model::vec2::dot(const model::vec2 &other) const noexcept {
	return x * other.x + y * other.y;
}

model::vec2 &model::vec2::unitify() {
	float n = norm();
	x /= n;
	y /= n;
	return *this;
}

model::vec2 model::vec2::unit() const noexcept {
	vec2 unit = *this;
	unit.unitify();
	return unit;
}

model::vec2 model::vec2::project_on(const model::vec2 &other) const noexcept {
	float norm = dot(other);
	return other.unit() * norm;
}

model::vec2 model::vec2::proj_point(const model::vec2 &other) const noexcept {
	return (dot(other) / other.dot(other)) * other;
}

float model::vec2::proj_coef(const model::vec2 &other) const noexcept {
	return dot(other) / other.dot(other);
}

model::vec2 model::vec2::normal() const noexcept {
	vec2 normal{y, x};
	normal.unitify();
	return normal;
}

float model::vec2::norm() const noexcept {
	return std::hypot(x, y);
}

model::vec2 model::vec2::to(const model::vec2 &other) const noexcept {
	return other - *this;
}

float model::vec2::prod(const model::vec2 &other) const noexcept {
	return x * other.y - y * other.x;
}

float model::vec2::atan2() const noexcept {
	return std::atan2(y, x);
}

std::ostream &operator<<(std::ostream &stream, const model::vec2 &vec) {
	stream << "(" << vec.x << ", " << vec.y << ")";
	return stream;
}
