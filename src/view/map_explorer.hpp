#ifndef NINJACLOWN_VIEW_MAP_EXPLORER_HPP
#define NINJACLOWN_VIEW_MAP_EXPLORER_HPP

#include <filesystem>
#include <cassert>

namespace utils {
class resource_manager;
}

namespace view {
class map_explorer {
public:
    void give_control(const utils::resource_manager& resources) noexcept;

    void open() noexcept {
        m_showing = true;
	}

    void close() {
        m_showing = false;
    }

    [[nodiscard]] bool map_ready() const noexcept {
        return m_has_map;
    }

    [[nodiscard]] std::filesystem::path selected_path() noexcept {
		assert(m_has_map);
		m_has_map = false;
        return std::exchange(m_selected_map, {});
    }

    [[nodiscard]] bool showing() const noexcept {
        return m_showing;
    }

private:
    bool m_showing{false};
    bool m_was_showing{false};
	bool m_has_map{false};
	std::filesystem::path m_selected_map{};
};
}

#endif //NINJACLOWN_MAP_EXPLORER_HPP
