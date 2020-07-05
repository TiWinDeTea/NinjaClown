#ifndef NINJACLOWN_CELL_HPP
#define NINJACLOWN_CELL_HPP

#include "model/interaction.hpp"
#include "model/types.hpp"
#include "utils/optional.hpp"

namespace model {

namespace cst {
	constexpr float cell_width  = 1.0f;
	constexpr float cell_height = 1.0f;
} // namespace cst

enum class cell_type {
	CHASM  = 1,
	GROUND = 2,
	WALL   = 3,
};

struct cell {
	cell() noexcept = default;

	explicit cell(cell_type type) noexcept
	    : type{type} { }

	cell(cell_type type, handle_t interaction_handle) noexcept
	    : type{type}
	    , interaction_handle{interaction_handle} { }

	cell_type type{cell_type::CHASM};
	utils::optional<handle_t> interaction_handle{};
};

} // namespace model

#endif //NINJACLOWN_CELL_HPP
