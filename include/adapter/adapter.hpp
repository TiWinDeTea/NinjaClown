#ifndef NINJACLOWN_ADAPTER_ADAPTER_HPP
#define NINJACLOWN_ADAPTER_ADAPTER_HPP

#include <filesystem>
#include <unordered_map>
#include <set>
#include <vector>
#include <variant>

namespace model {
enum class cell_type;
}

namespace state {
class holder;
}

namespace adapter {
struct model_handle {
	explicit model_handle() noexcept = default;
	explicit constexpr model_handle(size_t handle_) noexcept
	    : handle{handle_} {}
	constexpr bool operator==(const model_handle &other) const noexcept {
		return other.handle == handle;
	}
	size_t handle{};
};

struct view_handle {
	explicit view_handle() noexcept = default;
	explicit constexpr view_handle(bool is_mob_, size_t handle_) noexcept
	    : is_mob{is_mob_}
	    , handle{handle_} {}
	constexpr bool operator==(const view_handle &other) const noexcept {
		return other.is_mob == is_mob && other.handle == handle;
	}
	bool is_mob{};
	size_t handle{};
};

struct model_hhash: private std::hash<size_t> {
	size_t operator()(const model_handle &h) const noexcept {
		return std::hash<size_t>::operator()(h.handle);
	}
};
struct view_hhash {
	size_t operator()(const view_handle &h) const noexcept;
};

namespace request {
	struct coords {
		size_t x, y;
	};
	struct hitbox {
		float x, y;
		float width, height;
	};
} // namespace request
using draw_request = std::variant<std::monostate, request::coords, request::hitbox>;

class adapter {
public:
	explicit adapter(state::holder *state_holder) noexcept
	    : m_state{*state_holder} {}

	bool load_map(const std::filesystem::path &path) noexcept;

	void update_map(size_t y, size_t x, model::cell_type new_cell) noexcept;

	void move_entity(model_handle entity, float new_x, float new_y) noexcept;

	void rotate_entity(model_handle entity, float new_rad) noexcept;

	[[nodiscard]] draw_request tooltip_for(view_handle entity) noexcept;

public:
    std::vector<std::pair<size_t, size_t>> cells_changed_since_last_update{};

private:
	state::holder &m_state;

	std::unordered_map<model_handle, view_handle, model_hhash> m_model2view;
	std::unordered_map<view_handle, model_handle, view_hhash> m_view2model;
};
} // namespace adapter

#endif //NINJACLOWN_ADAPTER_ADAPTER_HPP
