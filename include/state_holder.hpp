#ifndef NINJACLOWN_STATE_HOLDER_HPP
#define NINJACLOWN_STATE_HOLDER_HPP

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>

#include <imterm/terminal.hpp>

#include "adapter/adapter.hpp"
#include "model/model.hpp"
#include "terminal_commands.hpp"
#include "utils/resource_manager.hpp"
#include "view/viewer.hpp"

namespace state {

template <typename>
class access;

struct property;

class holder {
	std::shared_ptr<terminal_commands> m_command_manager;

public:
	explicit holder(const std::filesystem::path &config) noexcept;

	void run() noexcept;

	void wait() noexcept;

	utils::resource_manager resources;

	std::map<std::string, property> properties;

private:
	ImTerm::terminal<terminal_commands> m_terminal;

	model::model m_model;
	view::viewer m_view;

	adapter::adapter m_adapter;

	friend terminal_commands;
	friend access<view::viewer>;
	friend access<adapter::adapter>;
	friend access<model::model>;
};

struct property {
	template <typename T>
	struct proxy {
		template <typename U>
		static proxy from_accessor(U &val, void (U::*set)(T), T (U::*get)() const) {
			return {[&val, set](const T &v) {
				        std::invoke(set, val, v);
			        },
			        [&val, get] () -> const T& {
				        return std::invoke(get, val);
			        }};
		}

		using SetterType = std::conditional_t<std::is_same_v<T, std::atomic_bool>, bool, T>;

		std::function<void(const SetterType &)> set;
		std::function<const T&()> get;
	};

	using settable_property = std::variant<proxy<unsigned int>, proxy<std::atomic_bool>>;
	using readonly_property = std::variant<float>;
	using any_property      = std::variant<settable_property, readonly_property>;

	template <typename T>
	property(proxy<T> &&proxy)
	    : data{[proxy = std::move(proxy)]() {
		    return any_property{settable_property{proxy}};
	    }} { }

	template <typename FuncT, typename... Args>
	property(FuncT &&funct, Args&... args) {
		using ResultT     = std::invoke_result_t<FuncT &, Args &...>;
		using ResultT_row = std::remove_const_t<std::remove_reference_t<ResultT>>;

		if constexpr (std::is_reference_v<ResultT>) {
			data = [funct = std::forward<FuncT>(funct), args = std::tie(args...)]() mutable {
				return any_property{settable_property{proxy<ResultT_row>{[&](const typename proxy<ResultT_row>::SetterType &val) mutable {
					                                                         std::apply(funct, args) = val;
				                                                         },
				                                                         [&]() -> const ResultT_row& {
					                                                         return std::apply(funct, args);
				                                                         }}}};
			};
		}
		else {
			data = [funct = std::forward<FuncT>(funct), args = std::tie(args...)]() {
				return any_property{readonly_property{std::apply(funct, args)}};
			};
		}
	}

	std::function<any_property(void)> data;
}; // namespace state

template <typename>
class access { };

template <>
class access<view::viewer> {
	static ImTerm::terminal<terminal_commands> &terminal(holder &holder) noexcept {
		return holder.m_terminal;
	}

	static adapter::adapter &adapter(holder &holder) noexcept {
		return holder.m_adapter;
	}

	friend view::viewer;
};

template <>
class access<model::model> {
	static adapter::adapter &adapter(holder &holder) noexcept {
		return holder.m_adapter;
	}

	friend model::model;
};

template <>
class access<adapter::adapter> {
	static model::model &model(holder &holder) noexcept {
		return holder.m_model;
	}

	static view::viewer &view(holder &holder) noexcept {
		return holder.m_view;
	}

	friend adapter::adapter;
};

} // namespace state

#endif //NINJACLOWN_STATE_HOLDER_HPP
