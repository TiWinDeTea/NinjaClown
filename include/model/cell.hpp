#ifndef NINJACLOWN_CELL_HPP
#define NINJACLOWN_CELL_HPP

#include <memory>
#include <optional>

#include "model/interactable.hpp"
#include "utils/optional.hpp"

namespace model {
enum class cell_type {
	VOID,
	GROUND,
	WALL,
};

class cell {
public:
	cell() noexcept {};

	explicit cell(cell_type type) noexcept
	    : m_type{type}
	{
	}

	cell(cell &&other) noexcept
	    : m_type{other.m_type}
	    , m_interactable{std::move(other.m_interactable)}
	{
	}

	cell(cell_type type, std::unique_ptr<interactable> &&interactable) noexcept
	    : m_type{type}
	    , m_interactable{std::move(interactable)}
	{
	}

	cell_type type() const
	{
		return m_type;
	}

	utils::optional<interactable &> get_interactable()
	{
		if (m_interactable) {
			return {*m_interactable.value()};
		}
		else {
			return {};
		}
	}

private:
	cell_type m_type{cell_type::VOID};
	std::optional<std::unique_ptr<interactable>> m_interactable{};
};
} // namespace model

#endif //NINJACLOWN_CELL_HPP
