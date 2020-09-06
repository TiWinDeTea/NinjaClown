#ifndef NINJACLOWN_VIEW_MENU_HPP
#define NINJACLOWN_VIEW_MENU_HPP

#include "view/file_explorer.hpp"
#include "view/configurator.hpp"

namespace state {
class holder;
}

namespace view {
class menu {
	enum class state {
		none,
		config,
		filesystem,
	};

	enum class substate {
		na,
		loading_dll,
	};

public:
	enum class user_request {
		none,
		close_menu,
		close_window,
		restart,
		load_dll,
	};


	explicit menu(::state::holder& state) noexcept;

	/**
	 * @return true on close request
	 */
	user_request show();

	void close();

	const std::filesystem::path& path() const noexcept {
		return m_path;
	}

private:

	::state::holder& m_state;
    configurator m_configurator;
    file_explorer m_explorer{};

    std::filesystem::path m_path{};

	state m_current_state{state::none};
	substate m_current_substate{substate::na};

	bool m_currently_open{false};

};
}

#endif //NINJACLOWN_VIEW_MENU_HPP
