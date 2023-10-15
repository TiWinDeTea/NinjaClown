#include <cassert>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/actionable.hpp"
#include "model/world.hpp"
#include "utils/logging.hpp"

using behaviours = model::actionable::behaviours;
using fmt::literals::operator""_a;

void behaviours::none(const instance_data &data, const argument_type &arg) noexcept {
	utils::log::info("actionable.none", "x"_a = data.pos.x, "y"_a = data.pos.y);
}

void behaviours::gate(const instance_data &data, const argument_type &arg) noexcept {
	assert(data.pos.x >= 0 && static_cast<std::size_t>(data.pos.x) < arg.world.map.width()); // NOLINT
	assert(data.pos.y >= 0 && static_cast<std::size_t>(data.pos.y) < arg.world.map.height()); // NOLINT

	switch (arg.world.map[data.pos.x][data.pos.y].type) {
		case cell_type::CHASM:
		case cell_type::WALL:
			utils::log::info("actionable.gate.open", "x"_a = data.pos.x, "y"_a = data.pos.y);
			arg.world.map[data.pos.x][data.pos.y].type = cell_type::GROUND;
			arg.adapter.open_gate(adapter::model_handle{data.handle, adapter::model_handle::ACTIONABLE});
			arg.adapter.update_map(data.pos, cell_type::GROUND);
			break;
		case cell_type::GROUND:
			utils::log::info("actionable.gate.close", "x"_a = data.pos.x, "y"_a = data.pos.y);
			arg.adapter.close_gate(adapter::model_handle{data.handle, adapter::model_handle::ACTIONABLE});
			arg.world.map[data.pos.x][data.pos.y].type = cell_type::WALL;
			arg.adapter.update_map(data.pos, cell_type::WALL);
			break;
	}
}

void behaviours::autoshooter(const instance_data &data, const argument_type &) noexcept {
	// TODO
	assert(false && "autoshooter behaviour is not implemented");
}
