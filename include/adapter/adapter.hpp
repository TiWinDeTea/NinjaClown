#ifndef NINJACLOWN_ADAPTER_ADAPTER_HPP
#define NINJACLOWN_ADAPTER_ADAPTER_HPP

#include <filesystem>
#include <set>
#include <unordered_map>
#include <variant>
#include <vector>

#include "utils/utils.hpp"
#include "model/grid_point.hpp"

namespace model {
enum class cell_type;
}

namespace state {
class holder;
}

namespace cpptoml {
class table;
}

namespace adapter {

struct model_handle {
	explicit model_handle() noexcept = default;
	explicit constexpr model_handle(size_t handle_) noexcept
	    : handle{handle_} { }
	constexpr bool operator==(const model_handle &other) const noexcept {
		return other.handle == handle;
	}
	size_t handle{};
};

struct view_handle {
	explicit view_handle() noexcept = default;
	explicit constexpr view_handle(bool is_mob_, size_t handle_) noexcept
	    : is_mob{is_mob_}
	    , handle{handle_} { }
	constexpr bool operator==(const view_handle &other) const noexcept {
		return other.is_mob == is_mob && other.handle == handle;
	}
	bool is_mob{};
	size_t handle{};
};

struct model_hhash: private std::hash<utils::ssize_t> {
	utils::ssize_t operator()(const model_handle &h) const noexcept {
		return std::hash<utils::ssize_t>::operator()(h.handle);
	}
};
struct view_hhash {
    std::size_t operator()(const view_handle &h) const noexcept;
};

namespace request {
	struct coords {
		utils::ssize_t x, y;
	};
	struct coords_list {
		std::vector<struct coords> coords;
	};

	struct hitbox {
		float x, y;
		float width, height;
	};
} // namespace request
using draw_request = std::variant<std::monostate, request::coords_list, request::hitbox>;

class adapter {
public:
	explicit adapter(state::holder *state_holder) noexcept
	    : m_state{*state_holder} { }

	bool load_map(const std::filesystem::path &path) noexcept;

	bool map_is_loaded() noexcept;

	void update_map(const model::grid_point &target, model::cell_type new_cell) noexcept;

	void move_entity(model_handle entity, float new_x, float new_y) noexcept;

	void rotate_entity(model_handle entity, float new_rad) noexcept;

	[[nodiscard]] draw_request tooltip_for(view_handle entity) noexcept;

public:
	std::vector<model::grid_point> cells_changed_since_last_update{};

private:
	bool load_map_v1_0_0(const std::shared_ptr<cpptoml::table> &tables, std::string_view map) noexcept;

	state::holder &m_state;

	std::unordered_map<model_handle, view_handle, model_hhash> m_model2view;
	std::unordered_map<view_handle, model_handle, view_hhash> m_view2model;
};
} // namespace adapter

#endif //NINJACLOWN_ADAPTER_ADAPTER_HPP
