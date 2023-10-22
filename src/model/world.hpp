#ifndef NINJACLOWN_WORLD_HPP
#define NINJACLOWN_WORLD_HPP

#include <algorithm> // max, min
#include <model/event.hpp>
#include <optional>
#include <string>
#include <vector>

#include "adapter/adapter.hpp"
#include "model/actionable.hpp"
#include "model/activator.hpp"
#include "model/cell.hpp"
#include "model/components.hpp"
#include "model/grid.hpp"
#include "model/interaction.hpp"

class terminal_commands;

namespace model {

struct world {
	world() = default;

	void update(adapter::adapter &);

	void reset();
	void reset_entity(handle_t);

	void move_entity(adapter::adapter &, handle_t, vec2 displacement);
	void rotate_entity(adapter::adapter &, handle_t, float rotation_rad);
	bool entity_check_collision(handle_t);

	void fire_activator(adapter::adapter &, handle_t, event_reason);
	void fire_actionable(adapter::adapter &, handle_t);

	[[nodiscard]] inline const grid &get_map() const noexcept {
		return m_map;
	}
	[[nodiscard]] inline grid &get_map() noexcept {
		return m_map;
	}

	[[nodiscard]] inline const components &get_components() const noexcept {
		return m_components;
	}
	[[nodiscard]] inline components &get_components() noexcept {
		return m_components;
	}

	[[nodiscard]] inline const std::vector<interaction> &get_interactions() const noexcept {
		return m_interactions;
	}
	[[nodiscard]] inline std::vector<interaction> &get_interactions() noexcept {
		return m_interactions;
	}

	[[nodiscard]] inline const std::vector<activator> &get_activators() const noexcept {
		return m_activators;
	}
	[[nodiscard]] inline std::vector<activator> &get_activators() noexcept {
		return m_activators;
	}

	[[nodiscard]] inline const std::vector<actionable> &get_actionables() const noexcept {
		return m_actionables;
	}
	[[nodiscard]] inline std::vector<actionable> &get_actionables() noexcept {
		return m_actionables;
	}

	[[nodiscard]] inline grid_point get_target_tile() const noexcept {
		return m_target_tile;
	}
	inline void set_target_tile(grid_point new_target_tile) noexcept {
		m_target_tile = new_target_tile;
	}

private:
	event_queue m_event_queue{};

	grid m_map{};

	components m_components{};

	std::vector<interaction> m_interactions{};
	std::vector<activator> m_activators{};
	std::vector<actionable> m_actionables{};

	grid_point m_target_tile{};
};

} // namespace model

#endif //NINJACLOWN_WORLD_HPP
