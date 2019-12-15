#ifndef NINJACLOWN_UTILS_HPP
#define NINJACLOWN_UTILS_HPP

#include <algorithm>
#include <string_view>

namespace utils {
inline bool stars_with(std::string_view str, std::string_view prefix)
{
	return std::mismatch(str.begin(), str.end(), prefix.begin(), prefix.end()).second == prefix.end();
}
} // namespace utils

#endif //NINJACLOWN_UTILS_HPP
