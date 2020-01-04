
namespace bot::details {

template <typename>
constexpr bool dependant_false = false;

constexpr void make_api_impl(bot_api &) {} // ends template recursion

template <typename Func, typename... OtherFuncs>
void make_api_impl(bot_api &api, Func f, OtherFuncs &&... others)
{
	if constexpr (std::is_same_v<Func, go_right>) {
		api.go_right = f.ptr;
	}
	else {
		static_assert(dependant_false<Func>, "unknown given type");
	}
	make_api_impl(api, std::forward<OtherFuncs>(others)...);
}
} // namespace bot::details

template <typename... Funcs>
bot::bot_api bot::make_api(Funcs &&... funcs)
{
	bot_api api;
	api.log      = &ffi::log;
	api.go_right = &ffi::go_right;
	details::make_api_impl(api, std::forward<Funcs>(funcs)...);
	return api;
}
