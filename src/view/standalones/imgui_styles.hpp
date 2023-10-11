#ifndef NINJACLOWN_IMGUI_STYLES_HPP
#define NINJACLOWN_IMGUI_STYLES_HPP

#include <utility>

namespace view {

namespace details {
	using void_void_fn_t = void (*)();

	struct style_store {
		void_void_fn_t push_style;
		void_void_fn_t pop_style;
	};

	template <typename FuncT>
	void operator+(style_store store, FuncT &&funct) noexcept(noexcept(funct())) {
		store.push_style();
		funct();
		store.pop_style();
	}

} // namespace details

void push_disabled_button_style() noexcept;
void pop_disabled_button_style() noexcept;

#define using_style(x) details::style_store{push_##x##_style, pop_##x##_style} + [&]()
} // namespace view

#endif //NINJACLOWN_IMGUI_STYLES_HPP
