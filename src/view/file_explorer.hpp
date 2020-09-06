#ifndef NINJACLOWN_FILE_EXPLORER_HPP
#define NINJACLOWN_FILE_EXPLORER_HPP

#include <filesystem>
#include <vector>

namespace utils {
class resource_manager;
}

namespace view {

struct with_extensions {

	with_extensions() noexcept = default;
	explicit with_extensions(std::initializer_list<std::string> init) : exts{std::move(init)} {}

	std::vector<std::string> exts{};
};

class file_explorer {

public:
	void give_control(const utils::resource_manager& resources) noexcept;

	void open(with_extensions = {}) noexcept;
	void open(std::filesystem::path path, with_extensions = {}) noexcept;

	void close() {
        m_currently_selected.clear();
		m_showing = false;
	}

	[[nodiscard]] bool path_ready() const noexcept {
		return m_path_ready;
	}

    [[nodiscard]] std::filesystem::path selected_path() noexcept {
		if (m_path_ready) {
			m_path_ready = false;
            m_currently_selected.clear();
		}
		return std::exchange(m_current, {});
	}

    [[nodiscard]] bool showing() const noexcept {
		return m_showing;
	}

private:
	std::filesystem::path m_current{};
	bool m_showing{false};
	bool m_path_ready{false};
	bool m_was_showing{false};

	std::filesystem::path m_currently_selected{};

	std::vector<std::string> m_prefered_extensions{};
};

}

#endif //NINJACLOWN_FILE_EXPLORER_HPP
