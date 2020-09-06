#ifndef NINJACLOWN_WORLD_HPP
#define NINJACLOWN_WORLD_HPP

#include <algorithm> // max, min
#include <model/event.hpp>
#include <optional>
#include <string>
#include <vector>

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

	grid_t grid{};

	::model::components components{};

	std::vector<interaction> interactions{};
	std::vector<activator> activators{};
	std::vector<actionable> actionables{};

	grid_point target_tile;

private:
	void single_entity_simple_update(adapter::adapter &, handle_t);
	void single_entity_decision_update(adapter::adapter &, handle_t);
	void single_entity_action_update(adapter::adapter &, handle_t);
	void move_entity(adapter::adapter &, handle_t, vec2 movement);
	void rotate_entity(adapter::adapter &, handle_t, float rotation_rad);
	bool entity_check_collision(handle_t);

	void fire_activator(adapter::adapter &, handle_t, event_reason);
	void fire_actionable(adapter::adapter &, handle_t);

	event_queue m_event_queue{};

	friend terminal_commands;
	friend event_queue;
};

} // namespace model

#endif //NINJACLOWN_WORLD_HPP
