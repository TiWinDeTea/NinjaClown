#ifndef NINJACLOWN_WORLD_HPP
#define NINJACLOWN_WORLD_HPP

#include <algorithm> // max, min
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
	world() noexcept = default;

	void update(adapter::adapter &);
	void reset();

	grid_t grid{};

	size_t ninja_clown_handle{};

	::model::components components{};

	std::vector<interaction> interactions{};
	std::vector<activator> activators{};
	std::vector<actionable> actionables{};

	grid_point target_tile;

private:
	void single_entity_simple_update(adapter::adapter &adapter, size_t handle);
	void move_entity(adapter::adapter &adapter, size_t handle, vec2 movement);
	void rotate_entity(adapter::adapter &adapter, size_t handle, float rotation_rad);
	bool entity_check_collision(size_t handle);

	void fire_activator(adapter::adapter &, size_t handle);
	void fire_actionable(adapter::adapter &, size_t handle);

	friend terminal_commands;
};

} // namespace model

#endif //NINJACLOWN_WORLD_HPP
