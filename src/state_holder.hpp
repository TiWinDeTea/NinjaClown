#ifndef NINJACLOWN_STATE_HOLDER_HPP
#define NINJACLOWN_STATE_HOLDER_HPP

#include <atomic>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>

class terminal_commands;

namespace ImTerm {
template <typename>
class terminal;
}

namespace utils {
class resource_manager;
}

namespace bot {
struct ffi;
}

namespace view {
class viewer;
}

namespace adapter {
class adapter;
}

namespace model {
class model;
}

namespace state {

template <typename>
class access;

struct property;

struct pimpl;
} // namespace state

// ------------------------------------------------------------------------------------------------------------------------------------------------

namespace state {

class holder {
public:
	holder(const std::filesystem::path &autorun_script) noexcept;
	~holder();

	void run() noexcept;

	void wait() noexcept;

	[[nodiscard]] utils::resource_manager &resources() noexcept;

	std::map<std::string, property> &properties() noexcept;

	const std::filesystem::path &current_map_path() noexcept;

private:
    void set_current_map_path(const std::filesystem::path&);

	std::unique_ptr<pimpl> m_pimpl;

	ImTerm::terminal<terminal_commands> &terminal() noexcept;
	adapter::adapter &adapter() noexcept;
	model::model &model() noexcept;
	view::viewer &view() noexcept;

	friend terminal_commands;
	friend access<view::viewer>;
	friend access<adapter::adapter>;
	friend access<model::model>;
	friend access<bot::ffi>;
};

struct property {
	template <typename T>
	struct proxy {
		template <typename U>
		static proxy from_accessor(U &val, void (U::*set)(T), T (U::*get)() const) {
			return {[&val, set](const T &v) {
				        std::invoke(set, val, v);
			        },
			        [&val, get]() -> T {
				        return std::invoke(get, val);
			        }};
		}

		using SetterType = std::conditional_t<std::is_same_v<T, std::atomic_bool>, bool, T>;

		std::function<void(const SetterType &)> set;
		std::function<const T &()> get;
	};

	using settable_property = std::variant<proxy<unsigned int>, proxy<std::atomic_bool>>;
	using readonly_property = std::variant<float>;
	using any_property      = std::variant<settable_property, readonly_property>;

	template <typename T>
	explicit property(proxy<T> &&proxy)
	    : data{[proxy = std::move(proxy)]() {
		    return any_property{settable_property{proxy}};
	    }} { }

	template <typename FuncT, typename... Args>
	explicit property(FuncT &&funct, Args &... args) {
		using ResultT     = std::invoke_result_t<FuncT &, Args &...>;
		using ResultT_row = std::remove_const_t<std::remove_reference_t<ResultT>>;

		if constexpr (std::is_reference_v<ResultT>) {
			data = [funct = std::forward<FuncT>(funct), args = std::tie(args...)]() mutable {
				return any_property{settable_property{proxy<ResultT_row>{[&](const typename proxy<ResultT_row>::SetterType &val) mutable {
					                                                         std::apply(funct, args) = val;
				                                                         },
				                                                         [&]() -> const ResultT_row & {
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
		return holder.terminal();
	}

	static adapter::adapter &adapter(holder &holder) noexcept {
		return holder.adapter();
	}

	friend view::viewer;
};

template <>
class access<model::model> {
	static adapter::adapter &adapter(holder &holder) noexcept {
		return holder.adapter();
	}

	friend model::model;
};

template <>
class access<adapter::adapter> {
	static model::model &model(holder &holder) noexcept {
		return holder.model();
	}

	static view::viewer &view(holder &holder) noexcept {
		return holder.view();
	}

	static void set_current_map_path(holder &holder, const std::filesystem::path &path) noexcept {
		holder.set_current_map_path(path);
	}

	friend adapter::adapter;
};

template <>
class access<bot::ffi> {
	static model::model &model(holder &holder) noexcept {
		return holder.model();
	}

	static adapter::adapter &adapter(holder &holder) noexcept {
		return holder.adapter();
	}

	friend bot::ffi;
};

} // namespace state

#endif //NINJACLOWN_STATE_HOLDER_HPP
