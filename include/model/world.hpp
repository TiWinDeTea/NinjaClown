#ifndef NINJACLOWN_WORLD_HPP
#define NINJACLOWN_WORLD_HPP

#include <algorithm> // max, min
#include <optional>
#include <string>
#include <vector>

#include "cell.hpp"
#include "components.hpp"
#include "grid.hpp"
#include "activator.hpp"
#include "interaction.hpp"

namespace model {

struct world {
	world() noexcept = default;

	void update(adapter::adapter &);

	grid_t grid{};

	size_t ninja_clown_handle{};

	::model::components components{};

	std::vector<interaction> interactions{};
	std::vector<activator> activators{};
	std::vector<actionable> actionables{};


private:
	void single_entity_simple_update(adapter::adapter &adapter, size_t handle);
	void move_entity(adapter::adapter &adapter, size_t handle, float x, float y);
	bool entity_check_collision(const component::hitbox &hitbox);
};

} // namespace model

#endif //NINJACLOWN_WORLD_HPP
