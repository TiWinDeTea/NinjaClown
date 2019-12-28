#ifndef NINJACLOWN_UTILS_UTILS_HPP
#define NINJACLOWN_UTILS_UTILS_HPP

#include <algorithm>
#include <mutex>
#include <string_view>
#include <charconv>
#include <optional>

namespace utils {
inline bool starts_with(std::string_view str, std::string_view prefix)
{
	return std::mismatch(str.begin(), str.end(), prefix.begin(), prefix.end()).second == prefix.end();
}

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
