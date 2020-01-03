#ifndef NINJACLOWN_UTILS_OPTIONAL_HPP
#define NINJACLOWN_UTILS_OPTIONAL_HPP

#include "utils.hpp"
#include <optional>
#include <type_traits>
#include <typeinfo>

namespace utils {

template <typename T>
struct [[nodiscard]] optional: std::optional<T>{};

template <typename T>
struct [[nodiscard]] optional<T &>
{
	using value_type = T &;

	constexpr optional() noexcept = default;
	constexpr optional(std::nullopt_t) noexcept {};

	constexpr optional(T & val) noexcept
	    : m_value{&val} {};

	template <typename U>
	optional(U & val)
	{
		static_assert(std::is_base_of_v<U, T> || std::is_base_of_v<T, U>);
		m_value = dynamic_cast<T *>(&val);
		if (m_value == nullptr) {
			throw std::bad_cast{};
		}
	}

	constexpr optional(const optional &other) noexcept
	    : m_value{other.m_value}
	{
	}

	template <typename U>
	optional(const optional<U *> &other)
	{
		static_assert(std::is_base_of_v<U, T> || std::is_base_of_v<T, U>);
		if (other.has_value()) {
			m_value = dynamic_cast<T *>(other.value);
			if (m_value == nullptr) {
				throw std::bad_cast{};
			}
		}
		else {
			m_value = nullptr;
		}
	}

	optional &operator=(std::nullopt_t) noexcept
	{
		m_value = nullptr;
		return *this;
	}
	constexpr optional &operator=(const optional &other)
	{
		m_value = other.value;
		return *this;
	}

	optional &operator=(T &val)
	{
		m_value = &val;
		return *this;
	}

	template <typename U>
	optional &operator=(U &val)
	{
		if constexpr (std::is_base_of_v<U, T> || std::is_base_of_v<T, U>) {
			m_value = dynamic_cast<T *>(&val);
			if (m_value == nullptr) {
				throw std::bad_cast{};
			}
		}
		else {
			static_assert(std::is_same_v<utils::remove_const<T>, U>, "Value is not assignable "
			                                                         "(no known conversion from U to T)");
			m_value = &val;
		}
		return *this;
	}

	template <class U>
	optional &operator=(const optional<U *> &other)
	{
		m_value = dynamic_cast<T *>(other.m_value);
		if (other.has_value() && m_value == nullptr) {
			throw std::bad_cast{};
		}
		return *this;
	}

	[[nodiscard]] constexpr T *operator->() noexcept
	{
		return m_value;
	}
	[[nodiscard]] constexpr const T *operator->() const noexcept
	{
		return m_value;
	}

	[[nodiscard]] constexpr T &operator*() noexcept
	{
		return *m_value;
	}
	[[nodiscard]] constexpr const T &operator*() const noexcept
	{
		return *m_value;
	}

	[[nodiscard]] constexpr explicit operator bool() const noexcept
	{
		return m_value != nullptr;
	}

	[[nodiscard]] constexpr bool has_value() const noexcept
	{
		return m_value != nullptr;
	}

	[[nodiscard]] constexpr T &value()
	{
		if (!has_value()) {
			throw std::bad_optional_access{};
		}
		return *m_value;
	}
	[[nodiscard]] constexpr const T &value() const
	{
		if (!has_value()) {
			throw std::bad_optional_access{};
		}
		return *m_value;
	}

	[[nodiscard]] constexpr T &value_or(T & default_value) const noexcept
	{
		return has_value() ? *m_value : default_value;
	}

	void swap(optional & other) noexcept
	{
		std::swap(other.m_value, m_value);
	}

	void reset() noexcept
	{
		m_value = nullptr;
	}

private:
	T *m_value{nullptr};
};

template <typename T>
[[nodiscard]] bool operator==(const optional<T *> &lhs, const optional<T *> &rhs)
{
	if (lhs.has_value()) {
		return rhs.has_value() && *lhs == *rhs;
	}
	else {
		return !rhs.has_value();
	}
}

template <typename T>
[[nodiscard]] bool operator==(const T &lhs, const optional<T *> &rhs)
{
	return rhs.has_value() && lhs == *rhs;
}

template <typename T>
[[nodiscard]] bool operator==(const optional<T *> &lhs, const T &rhs)
{
	return lhs.has_value() && *lhs == rhs;
}

template <typename T>
[[nodiscard]] bool operator<(const optional<T *> &lhs, const optional<T *> &rhs)
{
	return rhs.has_value() && (!lhs.has_value() || *lhs < *rhs);
}

template <typename T>
[[nodiscard]] bool operator<(const T &lhs, const optional<T *> &rhs)
{
	return rhs.has_value() && lhs < *rhs;
}

template <typename T>
[[nodiscard]] bool operator<(const optional<T *> &lhs, const T &rhs)
{
	return !lhs.has_value() || *lhs < rhs;
}

template <typename T>
[[nodiscard]] bool operator<=(const optional<T *> &lhs, const optional<T *> &rhs)
{
	return rhs.has_value() && (!lhs.has_value() || *lhs <= *rhs);
}
template <typename T>
[[nodiscard]] bool operator<=(const T &lhs, const optional<T *> &rhs)
{
	return rhs.has_value() && lhs <= *rhs;
}
template <typename T>
[[nodiscard]] bool operator<=(const optional<T *> &lhs, const T &rhs)
{
	return !lhs.has_value() || *lhs <= rhs;
}

template <typename T>
[[nodiscard]] bool operator>(const optional<T *> &lhs, const optional<T *> &rhs)
{
	return lhs.has_value() && (!rhs.has_value() || *lhs > *rhs);
}

template <typename T>
[[nodiscard]] bool operator>(const T &lhs, const optional<T *> &rhs)
{
	return !rhs.has_value() || lhs > *rhs;
}

template <typename T>
[[nodiscard]] bool operator>(const optional<T *> &lhs, const T &rhs)
{
	return lhs.has_value() && *lhs > rhs;
}

template <typename T>
[[nodiscard]] bool operator>=(const optional<T *> &lhs, const optional<T *> &rhs)
{
	return lhs.has_value() && (!rhs.has_value() || *lhs >= *rhs);
}
template <typename T>
[[nodiscard]] bool operator>=(const T &lhs, const optional<T *> &rhs)
{
	return lhs.has_value() && lhs >= *rhs;
}
template <typename T>
[[nodiscard]] bool operator>=(const optional<T *> &lhs, const T &rhs)
{
	return lhs.has_value() && *lhs >= rhs;
}

} // namespace utils

#endif //NINJACLOWN_UTILS_OPTIONAL_HPP
