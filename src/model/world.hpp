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

	grid map{};

	::model::components components{};

	std::vector<interaction> interactions{};
	std::vector<activator> activators{};
	std::vector<actionable> actionables{};

	grid_point target_tile;

private:
	void behavior_system(adapter::adapter &, handle_t);
	void decision_system(adapter::adapter &, handle_t);
	void action_system(adapter::adapter &, handle_t);
	void movement_system(adapter::adapter &, handle_t);

	void handle_projectile_behavior(adapter::adapter &, handle_t);

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
