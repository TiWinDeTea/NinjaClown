#include <cassert>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/actionable.hpp"
#include "model/world.hpp"

using behaviours_namespace = model::actionable::behaviours_ns;

// TODO externalize logs

void behaviours_namespace::none(const instance_data& data, const argument_type &arg) noexcept {
	spdlog::info("[{}, {}]: nothing to activate there.", data.pos.x, data.pos.y);
}

void behaviours_namespace::gate(const instance_data& data, const argument_type &arg) noexcept {
	assert(data.pos.x >= 0 && static_cast<std::size_t>(data.pos.x) < arg.world.grid.width()); // NOLINT
	assert(data.pos.y >= 0 && static_cast<std::size_t>(data.pos.y) < arg.world.grid.height()); // NOLINT

	switch (arg.world.grid[data.pos.x][data.pos.y].type) {
		case cell_type::CHASM:
		case cell_type::WALL:
			spdlog::info("Opening gate at [{}, {}]", data.pos.x, data.pos.y);
			arg.world.grid[data.pos.x][data.pos.y].type = cell_type::GROUND;
            arg.adapter.open_gate(adapter::model_handle{arg.handle, adapter::model_handle::ACTIONABLE});
            arg.adapter.update_map(data.pos, cell_type::GROUND);
			break;
		case cell_type::GROUND:
			spdlog::info("Closing gate at [{}, {}]", data.pos.x, data.pos.y);
			arg.adapter.close_gate(adapter::model_handle{arg.handle, adapter::model_handle::ACTIONABLE});
			arg.world.grid[data.pos.x][data.pos.y].type = cell_type::WALL;
            arg.adapter.update_map(data.pos, cell_type::WALL);
			break;
		case cell_type::TARGET:
			spdlog::warn("Attempting to activate gatedata.pos tile [{}, {}]", data.pos.x, data.pos.y);
			break;
	}
}

void behaviours_namespace::autoshooter(const instance_data& data, const argument_type &) noexcept {
	// TODO
	throw std::runtime_error("autoshooter behaviour is not implemented");
}
