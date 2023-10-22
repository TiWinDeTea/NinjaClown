#include <cmath>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/components.hpp"
#include "model/systems.hpp"
#include "model/world.hpp"
#include "utils/visitor.hpp"

const float SIGNIFICANT_ROTATION_EPSILON = 0.001f;

float slowdown_factor(const float r) {
	return -(0.3f * r * r) / 2.f - 0.1f * std::pow(std::cos(1.1f * r), 2.f) + 1.1f - 0.03f * r * r * std::cos(r);
}

void model::projectile_behaviour_system(const component::properties &properties, std::optional<component::movement> &movement) {
	movement = {
	  .rotation     = properties.rotation_speed,
	  .forward_diff = properties.move_speed,
	  .lateral_diff = 0.f,
	};
}

void model::patrol_behaviour_system(const component::properties &properties, std::optional<component::movement> &movement) {
	// TODO: follow a predefined path
}

void model::aggressive_behaviour_system(const component::properties &properties, std::optional<component::movement> &movement) {
	// TODO: rush to the player
}

void model::decision_system(const world &world, adapter::adapter &adapter, const handle_t current_entity,
                            const component::properties &properties, const component::hitbox &hitbox,
                            std::optional<component::decision> &decision, std::optional<component::movement> &movement,
                            component::state &state) {
	if (!decision.has_value()) {
		return;
	}

	// Cancel action currently being prepared when receiving another decision
	state.preparing_action   = {};
	state.ticks_before_ready = 0;

	utils::visitor decision_visitor{
	  [&](ninja_api::nnj_movement_request &mov_req) {
		  movement = {
		    .rotation     = mov_req.rotation,
		    .forward_diff = mov_req.forward_diff,
		    .lateral_diff = mov_req.lateral_diff,
		  };
	  },
	  [&](ninja_api::nnj_activate_request &activate_req) {
		  const cell &cell = world.get_map()[activate_req.column][activate_req.line];
		  if (cell.interaction_handle.has_value()) {
			  const interaction &interaction = world.get_interactions()[cell.interaction_handle.value()];
			  if (interaction.kind == interaction_kind::LIGHT_MANUAL || interaction.kind == interaction_kind::HEAVY_MANUAL) {
				  vec2 cell_center{activate_req.column, activate_req.line};
				  if (hitbox.center.to(cell_center).norm() <= properties.activate_range) {
					  state.preparing_action   = {activate_req};
					  state.ticks_before_ready = world.get_activators()[interaction.activator_handle].activation_difficulty;
				  }
			  }
		  }
	  },
	  [&](ninja_api::nnj_attack_request &attack_req) {
		  if (world.get_components().health[attack_req.target_handle]
		      && world.get_components().hitbox[attack_req.target_handle].has_value()) {
			  const component::hitbox &target_hitbox = world.get_components().hitbox[attack_req.target_handle].value();
			  if (hitbox.center.to(target_hitbox.center).norm() <= properties.attack_range) {
				  state.preparing_action   = {attack_req};
				  state.ticks_before_ready = properties.attack_delay;
			  }
		  }
	  },
	  [&](ninja_api::nnj_throw_request &throw_req) {
		  state.preparing_action   = {throw_req};
		  state.ticks_before_ready = properties.throw_delay;
	  }};

	std::visit(decision_visitor, decision.value());
	decision = {};

	adapter.mark_entity_as_dirty(current_entity);
}

void model::action_system(world &world, adapter::adapter &adapter, const handle_t current_entity, const component::properties &properties,
                          const component::hitbox &hitbox, component::state &state) {
	if (!state.preparing_action.has_value()) {
		return;
	}

	if (state.ticks_before_ready > 0) {
		state.ticks_before_ready -= 1;
		return;
	}

	utils::visitor action_visitor{
	  [&](ninja_api::nnj_activate_request &activate_req) {
		  const cell &cell = world.get_map()[activate_req.column][activate_req.line];
		  if (cell.interaction_handle.has_value()) {
			  const interaction &interaction = world.get_interactions()[cell.interaction_handle.value()];
			  if (interaction.kind == interaction_kind::LIGHT_MANUAL || interaction.kind == interaction_kind::HEAVY_MANUAL) {
				  vec2 cell_center{activate_req.column, activate_req.line};
				  if (hitbox.center.to(cell_center).norm() <= properties.activate_range) {
					  world.fire_activator(adapter, interaction.activator_handle, event_reason::ENTITY);
				  }
			  }
		  }
	  },
	  [&](ninja_api::nnj_attack_request &attack_req) {
		  std::optional<component::hitbox> &target_hitbox = world.get_components().hitbox[attack_req.target_handle];
		  std::optional<component::health> &target_health = world.get_components().health[attack_req.target_handle];

		  if (target_health.has_value() && target_hitbox.has_value()) {
			  if (hitbox.center.to(target_hitbox->center).norm() <= properties.attack_range) {
				  target_health->points -= 1;
				  if (target_health->points == 0) {
					  world.reset_entity(attack_req.target_handle);
					  adapter.hide_entity(adapter::model_handle{attack_req.target_handle, adapter::model_handle::ENTITY});
				  }
			  }
		  }
	  },
	  [&](ninja_api::nnj_throw_request &throw_req) {
		  // TODO: throw request (shuriken, etc)
	  },
	};

	std::visit(action_visitor, state.preparing_action.value());
	state.preparing_action = {};

	adapter.mark_entity_as_dirty(current_entity);
}

void model::movement_system(world &world, adapter::adapter &adapter, const handle_t current_entity, const component::properties &properties,
                            component::hitbox &hitbox, std::optional<component::movement> &movement, component::metadata &metadata) {
	if (!movement.has_value()) {
		return;
	}

	float rotation = std::clamp(movement->rotation, -properties.rotation_speed, properties.rotation_speed);
	if (std::abs(rotation) > SIGNIFICANT_ROTATION_EPSILON) {
		world.rotate_entity(adapter, current_entity, rotation);
	}

	float dx = std::cos(hitbox.rad) * movement->forward_diff + std::cos(hitbox.rad + uni::math::pi_2<float>) * movement->lateral_diff;
	float dy = -(std::sin(hitbox.rad) * movement->forward_diff + std::sin(hitbox.rad + uni::math::pi_2<float>) * movement->lateral_diff);
	vec2 displacement{dx, dy};

	if (displacement.norm() != 0) {
		// maximal speed is achieved by fully moving forward, otherwise entity is slowed down
		float max_norm = properties.move_speed * slowdown_factor(hitbox.rad + displacement.atan2());

		if (displacement.norm() > max_norm) {
			// cap displacement vector to max speed
			displacement.unitify();
			displacement.x *= max_norm;
			displacement.y *= max_norm;
		}
		world.move_entity(adapter, current_entity, displacement);
	}

	movement = {};
	adapter.mark_entity_as_dirty(current_entity);

	// Check for victory
	if (metadata.kind == ninja_api::nnj_entity_kind::EK_DLL) {
		grid_point target_tile = world.get_target_tile();

		float distance
		  = hitbox.center
		      .to({static_cast<float>(target_tile.x) + cst::cell_width / 2.f, static_cast<float>(target_tile.y) + cst::cell_height / 2.f})
		      .norm();

		if (distance < hitbox.height() && distance < hitbox.width()) {
			spdlog::info("You win.");
			// TODO: send signal to UI and stop simulation
		}
	}
}
