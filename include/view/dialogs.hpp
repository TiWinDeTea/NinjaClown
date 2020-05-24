#ifndef NINJACLOWN_VIEW_DIALOGS_HPP
#define NINJACLOWN_VIEW_DIALOGS_HPP

#include <atomic>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "utils/synchronized.hpp"

namespace view {

struct word {
	std::string author{};
	std::string sentence{};
};

struct dialog {
	std::vector<word> words{};
	std::vector<word>::size_type current_word{};
};

class dialog_viewer {
	using time_point = std::chrono::time_point<std::chrono::system_clock>;
public:
	bool on_click(int x, int y) noexcept;

	void show(unsigned int win_length, unsigned int win_height) noexcept;

	void set_dialogs(std::vector<dialog> &&dialogs) noexcept {
		m_dialogs = std::move(dialogs);
	}

	void dll_word(std::string word, std::chrono::milliseconds duration) noexcept;

	void select_dialog(std::vector<dialog>::size_type dialog_idx) noexcept;

	void clear();

private:
	std::atomic_bool m_dll_word_dirty{false};
	utils::synchronized<std::pair<std::string, time_point>> m_dll_word_synced{};
	std::pair<std::string, time_point> m_dll_word{};

	std::vector<dialog> m_dialogs{};
	std::optional<std::vector<dialog>::size_type> m_current_dialog{};
};
} // namespace view

#endif //NINJACLOWN_VIEW_DIALOGS_HPP
