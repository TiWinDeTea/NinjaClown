#ifndef NINJACLOWN_UTILS_UNIVERSAL_CONSTANTS_HPP
#define NINJACLOWN_UTILS_UNIVERSAL_CONSTANTS_HPP

namespace uni::math {

namespace details {
	template <typename>
	void undefined();
}

template <typename T>
constexpr T pi = details::undefined<T>();
template <>
inline constexpr auto pi<double> = 3.1415926535897932384626433832795028841971693993751058209749445923078164062;
template <>
inline constexpr auto pi<float> = 3.1415926535897932384626433832795028841971693993751058209749445923078164062f;

template <typename T>
constexpr auto pi_2 = pi<T> / 2;
template <typename T>
constexpr auto pi_4 = pi<T> / 4;

template <typename T>
constexpr T exp = details::undefined<T>();
template <>
inline constexpr auto exp<double> = 2.718281828459045235360287471352662497757247093699959574966;
template <>
inline constexpr auto exp<float> = 2.718281828459045235360287471352662497757247093699959574966f;

} // namespace uni::math

#endif //NINJACLOWN_UTILS_UNIVERSAL_CONSTANTS_HPP
