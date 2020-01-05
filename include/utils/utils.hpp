#ifndef NINJACLOWN_UTILS_UTILS_HPP
#define NINJACLOWN_UTILS_UTILS_HPP

#include <algorithm>
#include <charconv>
#include <functional>
#include <optional>
#include <string_view>
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
struct add_const_s<T &> {
	using type = T const &;
};

template <typename T>
using add_const = typename add_const_s<T>::type;

template <typename T>
struct remove_const_s {
	using type = std::remove_const_t<T>;
};

template <typename T>
struct remove_const_s<T &> {
	using type = std::remove_const_t<T> &;
};

template <typename T>
using remove_const = typename remove_const_s<T>::type;

template <typename T>
std::optional<T> from_chars(std::string_view str)
{
	T value;
	auto result = std::from_chars(str.data(), str.data() + str.size(), value);
	if (result.ptr != str.data() + str.size() || result.ec != std::errc{}) {
		return {};
	}
	return value;
}

// std::invoke is not constexpr in c++17
namespace detail {
	template <class T>
	struct is_reference_wrapper: std::false_type {
	};
	template <typename U>
	struct is_reference_wrapper<std::reference_wrapper<U>>: std::true_type {
	};
	template <typename T>
	constexpr bool is_reference_wrapper_v = is_reference_wrapper<T>::value;

	template <typename T, typename Type, typename T1, typename... Args>
	constexpr decltype(auto) do_invoke(Type T::*f, T1 &&t1, Args &&... args)
	{
		if constexpr (std::is_member_function_pointer_v<decltype(f)>) {
			if constexpr (std::is_base_of_v<T, std::decay_t<T1>>)
				return (std::forward<T1>(t1).*f)(std::forward<Args>(args)...);
			else if constexpr (is_reference_wrapper_v<std::decay_t<T1>>)
				return (t1.get().*f)(std::forward<Args>(args)...);
			else
				return ((*std::forward<T1>(t1)).*f)(std::forward<Args>(args)...);
		}
		else {
			static_assert(std::is_member_object_pointer_v<decltype(f)>);
			static_assert(sizeof...(args) == 0);
			if constexpr (std::is_base_of_v<T, std::decay_t<T1>>)
				return std::forward<T1>(t1).*f;
			else if constexpr (is_reference_wrapper_v<std::decay_t<T1>>)
				return t1.get().*f;
			else
				return (*std::forward<T1>(t1)).*f;
		}
	}

	template <class F, class... Args>
	constexpr decltype(auto) do_invoke(F &&f, Args &&... args)
	{
		return std::forward<F>(f)(std::forward<Args>(args)...);
	}
} // namespace detail

template <class F, class... Args>
constexpr std::invoke_result_t<F, Args...> invoke(F &&f, Args &&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>)
{
	return detail::do_invoke(std::forward<F>(f), std::forward<Args>(args)...);
}

// std::not_fn is not constexpr in c++17
namespace details {

	template <typename Func>
	struct not_fn_t {
		template <typename... Args>
		constexpr auto operator()(Args &&... args) & -> decltype(!std::declval<std::invoke_result_t<std::decay_t<Func> &, Args...>>())
		{
			return !utils::invoke(f, std::forward<Args>(args)...);
		}

		template <class... Args>
		constexpr auto
		operator()(Args &&... args) const & -> decltype(!std::declval<std::invoke_result_t<std::decay_t<Func> const &, Args...>>())
		{
			return !utils::invoke(f, std::forward<Args>(args)...);
		}

		template <class... Args>
		constexpr auto operator()(Args &&... args) &&

		  -> decltype(!std::declval<std::invoke_result_t<std::decay_t<Func>, Args...>>())
		{
			return !utils::invoke(std::move(f), std::forward<Args>(args)...);
		}

		template <class... Args>
		constexpr auto
		operator()(Args &&... args) const && -> decltype(!std::declval<std::invoke_result_t<std::decay_t<Func> const, Args...>>())
		{
			return !utils::invoke(std::move(f), std::forward<Args>(args)...);
		}

		Func f;
	};
} // namespace details

template <typename Func>
constexpr auto not_fn(Func &&f) noexcept
{
	return details::not_fn_t<Func>{std::forward<Func>(f)};
}

// std::is_sorted is not constexpr in c++17
template <typename ForwardIt, typename Compare>
constexpr bool is_sorted(ForwardIt begin, ForwardIt end, Compare comp)
{
	if (begin == end) {
		return true;
	}
	auto current  = std::next(begin);
	auto previous = begin;

	while (current != end) {
		if (!comp(*previous++, *current++)) {
			return false;
		}
	}
	return true;
}

template <typename ForwardIt>
constexpr bool is_sorted(ForwardIt begin, ForwardIt end)
{
	return utils::is_sorted(begin, end, std::less<void>{});
}

// std::unique is not constexpr in c++17
template <typename ForwardIt, typename Predicate>
constexpr bool unique(ForwardIt begin, ForwardIt end, Predicate pred)
{
	return utils::is_sorted(begin, end, utils::not_fn(pred));
}

// std::unique is not constexpr in c++17
template <typename ForwardIt>
constexpr bool unique(ForwardIt begin, ForwardIt end)
{
	return utils::is_sorted(begin, end, utils::not_fn(std::equal_to<void>{}));
}

// checks that a collection contains all number sorted from 0 to max, exactly once
template <typename T>
constexpr bool has_all_sorted(const T &values, typename T::value_type max)
{
	return values.size() == max + 1 && values.back() == max && utils::is_sorted(values.begin(), values.end())
	       && utils::unique(values.begin(), values.end());
}

} // namespace utils

#endif //NINJACLOWN_UTILS_UTILS_HPP
