#ifndef NINJACLOWN_UTILS_HPP
#define NINJACLOWN_UTILS_HPP

#include <string_view>
#include <algorithm>

namespace utils {
inline bool stars_with(std::string_view str, std::string_view prefix) {
	return std::mismatch(str.begin(), str.end(), prefix.begin(), prefix.end()).second == prefix.end();
}
}

#endif //NINJACLOWN_UTILS_HPP
