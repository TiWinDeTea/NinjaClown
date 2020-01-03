#ifndef NINJACLOWN_UTILS_UTILS_HPP
#define NINJACLOWN_UTILS_UTILS_HPP

#include <algorithm>
#include <string_view>
#include <charconv>
#include <optional>
#include <type_traits>

namespace utils {
inline bool starts_with(std::string_view str, std::string_view prefix)
{
	return std::mismatch(str.begin(), str.end(), prefix.begin(), prefix.end()).second == prefix.end();
}

template <typename T>
struct add_const_s {
    using type = T const;
};

template <typename T>
struct add_const_s<T&> {
    using type = T const &;
};

template <typename T>
using add_const = typename add_const_s<T>::type;

template <typename T>
struct remove_const_s {
    using type = std::remove_const_t<T>;
};

template <typename T>
struct remove_const_s<T&> {
    using type = std::remove_const_t<T>&;
};

template <typename T>
using remove_const = typename remove_const_s<T>::type;

template <typename T>
std::optional<T> from_chars(std::string_view str) {
    T value;
    auto result = std::from_chars(str.data(), str.data() + str.size(), value);
    if (result.ptr != str.data() + str.size() || result.ec != std::errc{}) {
        return {};
    }
    return value;
}
} // namespace utils

#endif //NINJACLOWN_UTILS_UTILS_HPP
