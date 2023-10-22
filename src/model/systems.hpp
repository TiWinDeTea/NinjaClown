#ifndef NINJACLOWN_SYSTEMS_HPP
#define NINJACLOWN_SYSTEMS_HPP

#include "adapter/adapter.hpp"
#include "model/components.hpp"

namespace model {

class world; // forward declaration

void projectile_behaviour_system(const component::properties &, std::optional<component::movement> &);

void patrol_behaviour_system(const component::properties &, std::optional<component::movement> &);

void aggressive_behaviour_system(const component::properties &, std::optional<component::movement> &);

void decision_system(const world &, adapter::adapter &, handle_t current_entity, const component::properties &, const component::hitbox &,
                     std::optional<component::decision> &, std::optional<component::movement> &, component::state &);

void action_system(world &, adapter::adapter &, handle_t current_entity, const component::properties &, const component::hitbox &,
                   component::state &);

void movement_system(world &, adapter::adapter &, handle_t current_entity, const component::properties &, component::hitbox &,
                     std::optional<component::movement> &, component::metadata &);

} // namespace model

#endif //NINJACLOWN_SYSTEMS_HPP
