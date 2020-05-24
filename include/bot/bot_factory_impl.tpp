
namespace bot::details {

template <typename>
constexpr bool dependant_false = false;

constexpr void make_api_impl(bot_api &) { } // ends template recursion

template <typename Func, typename... OtherFuncs>
void make_api_impl(bot_api &api, Func f, OtherFuncs &&... others) {
	if constexpr (std::is_same_v<Func, commit_decision>) {
		api.commit_decision = f.ptr;
	}
	else {
		static_assert(dependant_false<Func>, "unknown given type");
	}
	make_api_impl(api, std::forward<OtherFuncs>(others)...);
}
} // namespace bot::details

template <typename... Funcs>
bot::bot_api bot::make_api(Funcs &&... funcs) {
	bot_api api{};

	api.log = &ffi::log;

	api.map_width  = &ffi::map_width;
	api.map_height = &ffi::map_height;
	api.map_scan   = &ffi::map_scan;
	api.map_update = &ffi::map_update;

	api.max_entities    = &ffi::max_entities;
	api.entities_scan   = &ffi::entities_scan;
	api.entities_update = &ffi::entities_update;

	api.commit_decision = &ffi::commit_decision;

	details::make_api_impl(api, std::forward<Funcs>(funcs)...);

	return api;
}
