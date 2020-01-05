#ifndef NINJACLOWN_CELL_HPP
#define NINJACLOWN_CELL_HPP

#include <optional>

#include "model/interaction.hpp"

namespace model {

enum class cell_type {
	CHASM  = 0,
	GROUND = 1,
	WALL   = 2,
};

struct cell {
	cell() noexcept = default;

	explicit cell(cell_type type) noexcept
	    : type{type}
	{
	}

	cell(cell &&other) noexcept
	    : type{other.type}
	    , interaction_handle{other.interaction_handle}
	{
	}

	cell(cell_type type, size_t interaction_handle) noexcept
	    : type{type}
	    , interaction_handle{interaction_handle}
	{
	}

	cell_type type{cell_type::CHASM};
	std::optional<size_t> interaction_handle{};
};

} // namespace model

#endif //NINJACLOWN_CELL_HPP
