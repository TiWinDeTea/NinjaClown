
namespace bot::details {

template <typename>
constexpr bool dependant_false = false;

constexpr void make_api_impl(bot_api &) {} // ends template recursion

template <typename Func, typename... OtherFuncs>
void make_api_impl(bot_api &api, Func f, OtherFuncs &&... others) {
	if constexpr (std::is_same_v<Func, move_backward>) {
		api.move_backward = f.ptr;
	}
	else {
		static_assert(dependant_false<Func>, "unknown given type");
	}
	make_api_impl(api, std::forward<OtherFuncs>(others)...);
}
} // namespace bot::details

template <typename... Funcs>
bot::bot_api bot::make_api(Funcs &&... funcs) {
	bot_api api;

	api.log = &ffi::log;

	api.vision = &ffi::vision;

	api.get_angle      = &ffi::get_angle;
	api.get_x_position = &ffi::get_x_position;
	api.get_y_position = &ffi::get_y_position;

	api.turn_left     = &ffi::turn_left;
	api.turn_right    = &ffi::turn_right;
	api.move_forward  = &ffi::move_forward;
	api.move_backward = &ffi::move_backward_dummy;

	api.activate_button = &ffi::activate_button;

	details::make_api_impl(api, std::forward<Funcs>(funcs)...);

	return api;
}
