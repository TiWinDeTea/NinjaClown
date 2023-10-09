#ifndef NINJACLOWN_VIEW_CONFIGURATOR_HPP
#define NINJACLOWN_VIEW_CONFIGURATOR_HPP

namespace utils {
class resource_manager;
}

namespace view {

class configurator {
public:
	explicit configurator() noexcept {}

	void give_control() noexcept;

	void show() noexcept {
		m_showing = true;
	}

	void close() noexcept {
		m_showing = false;
	}

	[[nodiscard]] bool showing() noexcept {
		return m_showing && m_popup_open;
	}

	bool were_graphics_changed() const noexcept {
		return m_graphics_changed;
	}

private:
	bool m_popup_open{false};
	bool m_showing{false};

	bool m_config_must_be_saved{false};
	bool m_graphics_changed{false};
};
} // namespace view

#endif //NINJACLOWN_VIEW_CONFIGURATOR_HPP
