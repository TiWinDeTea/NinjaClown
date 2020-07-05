#ifndef NINJACLOWN_UTILS_SCOPE_GUARDS_HPP
#define NINJACLOWN_UTILS_SCOPE_GUARDS_HPP

#include <exception>
#include <utility>

#include "macros.hpp"

namespace utils::details::scope_guards {
template <typename FuncT>
struct always {

	explicit constexpr always(FuncT &&f) noexcept(noexcept(FuncT{std::declval<FuncT &&>()}))
	    : m_funct{std::move(f)} {}

	explicit constexpr always(const FuncT &f) noexcept(noexcept(FuncT{std::declval<const FuncT &>()}))
	    : m_funct{f} {}

	always(const always &) = delete;

	always(always &&) = delete;

	always &operator=(const always &) = delete;

	always &operator=(always &&) = delete;

	~always() {
		m_funct();
	}

private:
	FuncT m_funct;
};

template <typename FuncT>
struct success {

	explicit constexpr success(FuncT &&f) noexcept(noexcept(FuncT{std::declval<FuncT &&>()}))
	    : m_funct{std::move(f)} {}

	explicit constexpr success(const FuncT &f) noexcept(noexcept(FuncT{std::declval<const FuncT &>()}))
	    : m_funct{f} {}

	success(const success &) = delete;

	success(success &&) = delete;

	success &operator=(const success &) = delete;

	success &operator=(success &&) = delete;

	~success() {
		if (m_exception_count >= std::uncaught_exceptions()) {
			m_funct();
		}
	}

private:
	FuncT m_funct;
	int m_exception_count{std::uncaught_exceptions()};
};

template <typename FuncT>
struct failed {

	explicit constexpr failed(FuncT &&f) noexcept(noexcept(FuncT{std::declval<FuncT &&>()}))
	    : m_funct{std::move(f)} {}

	explicit constexpr failed(const FuncT &f) noexcept(noexcept(FuncT{std::declval<const FuncT &>()}))
	    : m_funct{f} {}

	failed(const failed &) = delete;

	failed(failed &&) = delete;

	failed &operator=(const failed &) = delete;

	failed &operator=(failed &&) = delete;

	~failed() {
		if (m_exception_count < std::uncaught_exceptions()) {
			m_funct();
		}
	}

private:
	FuncT m_funct;
	int m_exception_count{std::uncaught_exceptions()};
};

struct make_always_t {};
struct make_success_t {};
struct make_failed_t {};

[[maybe_unused]] constexpr make_always_t make_always{};
[[maybe_unused]] constexpr make_success_t make_success{};
[[maybe_unused]] constexpr make_failed_t make_failed{};

inline void test() {}

template <typename FuncT>
always<FuncT> operator+(make_always_t /* placeholder */, FuncT &&funct) {
	return always<FuncT>{funct};
}

template <typename FuncT>
success<FuncT> operator+(make_success_t /* placeholder */, FuncT &&funct) {
	return success<FuncT>{funct};
}

template <typename FuncT>
failed<FuncT> operator+(make_failed_t /* placeholder */, FuncT &&funct) {
	return failed<FuncT>{funct};
}
} // namespace utils::details::scope_guards

#define NINJACLOWN_MAKE_ON_SCOPE(x)                                                                                                        \
	[[maybe_unused]] auto NINJACLOWN_MAKE_VAR_NAME = NINJACLOWN_CONCAT(utils::details::scope_guards::make_, x) + [&]()

#define ON_SCOPE_EXIT         NINJACLOWN_MAKE_ON_SCOPE(always)
#define ON_SCOPE_EXIT_SUCCESS NINJACLOWN_MAKE_ON_SCOPE(success)
#define ON_SCOPE_EXIT_FAILED  NINJACLOWN_MAKE_ON_SCOPE(failed)

#endif // NINJACLOWN_UTILS_SCOPE_GUARDS_HPP
