#ifndef NINJACLOWN_ADAPTER_ADAPTER_HPP
#define NINJACLOWN_ADAPTER_ADAPTER_HPP

#include <filesystem>
#include <set>
#include <unordered_map>
#include <variant>
#include <vector>

#include "model/grid_point.hpp"
#include "model/types.hpp"
#include "utils/utils.hpp"

class terminal_commands;

namespace model {
enum class cell_type;
}

namespace state {
class holder;
}

namespace cpptoml {
class table;
}

namespace utils {
class resource_manager;
}

namespace adapter {

enum class bot_log_level {
	TRACE    = 0,
	DEBUG    = 1,
	INFO     = 2,
	WARN     = 3,
	ERROR    = 4,
	CRITICAL = 5,
};

struct model_handle {
	enum type_t {
		ACTIVATOR  = 1,
		ACTIONABLE = 2,
		ENTITY     = 4,
	};

	explicit model_handle() noexcept = default;
	explicit constexpr model_handle(model::handle_t handle_, type_t type_) noexcept
	    : handle{handle_}
	    , type{type_} { }
	constexpr bool operator==(const model_handle &other) const noexcept {
		return other.handle == handle && type == other.type;
	}

	model::handle_t handle{};
	type_t type{};
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
		return std::hash<utils::ssize_t>::operator()(h.handle | h.type << 16u);
	}
};
struct view_hhash {
	std::size_t operator()(const view_handle &h) const noexcept;
};

namespace request {
	struct coords {
		utils::ssize_t x, y;
	};

	struct hitbox {
		float x, y;
		float width, height;
	};

	struct info {
		std::vector<std::string> lines;
	};
} // namespace request
using draw_request = std::vector<std::variant<request::coords, request::hitbox, request::info>>;

class adapter {
public:
	explicit adapter(state::holder *state_holder) noexcept
	    : m_state{*state_holder} { }

	// -- Used by view -- //

	bool load_map(const std::filesystem::path &path) noexcept;

	bool map_is_loaded() noexcept;

	[[nodiscard]] draw_request tooltip_for(view_handle entity) noexcept;

	// -- Used by model -- //

	void fire_activator(model_handle handle) noexcept;

	void close_gate(model_handle gate) noexcept;
	void open_gate(model_handle gate) noexcept;

	void update_map(const model::grid_point &target, model::cell_type new_cell) noexcept;
	void clear_cells_changed_since_last_update() noexcept;

	void move_entity(model_handle entity, float new_x, float new_y) noexcept;
	void hide_entity(model_handle entity) noexcept;
	void rotate_entity(model_handle entity, float new_rad) noexcept;
	void mark_entity_as_dirty(model::handle_t) noexcept; // QUESTION: what’s the purpose of this?
	void clear_entities_changed_since_last_update() noexcept;

	[[nodiscard]] utils::resource_manager &resources();

	// -- Used by bot / dll -- //

	const std::vector<model::grid_point> &cells_changed_since_last_update() noexcept;
	const std::vector<std::size_t> &entities_changed_since_last_update() noexcept;

	void bot_log(bot_log_level level, const char *text);

private:
	friend terminal_commands;

	bool load_map_v1_0_0(const std::shared_ptr<cpptoml::table> &tables, std::string_view map) noexcept;

	state::holder &m_state;

	std::optional<view_handle> m_target_handle{};
	std::unordered_map<model_handle, view_handle, model_hhash> m_model2view;
	std::unordered_map<view_handle, model_handle, view_hhash> m_view2model;
	std::unordered_map<view_handle, std::string, view_hhash> m_view2name;

	std::vector<model::grid_point> m_cells_changed_since_last_update{};
	std::vector<std::size_t> m_entities_changed_since_last_update{};
};
} // namespace adapter

#endif //NINJACLOWN_ADAPTER_ADAPTER_HPP
