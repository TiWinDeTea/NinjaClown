#ifndef NINJACLOWN_MODEL_ACTIVABLE_HPP
#define NINJACLOWN_MODEL_ACTIVABLE_HPP

#include <cassert>

#include "utils/utils.hpp"
#include "model/grid_point.hpp"

namespace adapter {
class adapter;
}

namespace model {

struct actionable {

	struct argument_type {
		size_t handle;
		grid_point source;
        struct world& world;
		adapter::adapter& adapter;
	};

	struct instance_data {
		grid_point pos;
	};

	using behaviour_type = void (*)(const instance_data&, const argument_type &) noexcept;
	struct behaviours_ns {
		static void none(const instance_data&, const argument_type &) noexcept;
		static void gate(const instance_data&, const argument_type &) noexcept;
		static void autoshooter(const instance_data&, const argument_type &) noexcept;
	};

	void make_action(const argument_type &arg) const noexcept {
        assert(behaviour); // NOLINT
        behaviour(data, arg);
	}

	instance_data data{};
	behaviour_type behaviour{behaviours_ns::none};
};
}; // namespace model

#endif //NINJACLOWN_MODEL_ACTIVABLE_HPP
