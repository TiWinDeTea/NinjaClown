#include <array>
#include <cmath>
#include <model/event.hpp>
#include <spdlog/spdlog.h>
#include <variant>

#include "adapter/adapter.hpp"
#include "model/collision.hpp"
#include "model/components.hpp"
#include "model/world.hpp"
#include "ninja_clown/api.h"
#include "utils/visitor.hpp"

#define ENSURE_COMPONENT(HANDLE, COMPONENT)                                                                                                \
	if (!components.COMPONENT[(HANDLE)].has_value()) {                                                                                     \
		return;                                                                                                                            \
	}
#define ENSURE_COMPONENT_OR_ELSE(HANDLE, COMPONENT, ELSE_BLOCK)                                                                            \
	if (!components.COMPONENT[(HANDLE)].has_value())                                                                                       \
	ELSE_BLOCK

const float SIGNIFICANT_ROTATION_EPSILON = 0.001f;

float slowdown_factor(float r);

void model::world::update(adapter::adapter &adapter) {
	for (handle_t handle = cst::max_entities; (handle--) != 0u;) {
		behavior_system(adapter, handle);
		decision_system(adapter, handle);
		action_system(adapter, handle);
		movement_system(adapter, handle);
	}

	m_event_queue.update(*this, adapter);
}

void model::world::reset() {
	map.resize(0, 0);
	interactions.clear();
	activators.clear();
	actionables.clear();

	for (unsigned int i = 0; i < cst::max_entities; ++i) {
		reset_entity(i);
	}
}

void model::world::reset_entity(handle_t handle) {
	components.state[handle]      = {};
	components.metadata[handle]   = {};
	components.properties[handle] = {};
	components.movement[handle].reset();
	components.decision[handle].reset();
	components.health[handle].reset();
	components.hitbox[handle].reset();
}

// -- Systems -- //

void model::world::behavior_system(adapter::adapter &adapter, handle_t handle) {
	switch (components.metadata[handle].kind) {
		case ninja_api::nnj_entity_kind::EK_PATROL:
			// TODO: follow a predefined path
			break;

		case ninja_api::nnj_entity_kind::EK_AGGRESSIVE:
			// TODO: rush to the player
			break;

		case ninja_api::nnj_entity_kind::EK_PROJECTILE:
			handle_projectile_behavior(adapter, handle);
			break;

		case ninja_api::nnj_entity_kind::EK_DLL:
		case ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY:
		case ninja_api::nnj_entity_kind::EK_HARMLESS:
		default:
			break;
	}
}

void model::world::handle_projectile_behavior(adapter::adapter &adapter, handle_t handle) {
	component::movement movement = {
	  .rotation     = components.properties[handle].rotation_speed,
	  .forward_diff = components.properties[handle].move_speed,
	  .lateral_diff = 0.f,
	};

	components.movement[handle] = {movement};

	adapter.mark_entity_as_dirty(handle);
}

void model::world::decision_system(adapter::adapter &adapter, handle_t handle) {
	ENSURE_COMPONENT(handle, decision);
	ENSURE_COMPONENT(handle, hitbox);

	component::decision &decision = *components.decision[handle];
	component::hitbox &hitbox     = *components.hitbox[handle];
	component::state &state       = components.state[handle];

	// Cancel action currently being prepared when receiving another decision
	state.preparing_action   = {};
	state.ticks_before_ready = 0;

	utils::visitor decision_visitor{
	  [&](ninja_api::nnj_movement_request &mov_req) {
		  components.movement[handle] = {
		    .rotation     = mov_req.rotation,
		    .forward_diff = mov_req.forward_diff,
		    .lateral_diff = mov_req.lateral_diff,
		  };
	  },
	  [&](ninja_api::nnj_activate_request &activate_req) {
		  const cell &cell = map[activate_req.column][activate_req.line];
		  if (cell.interaction_handle.has_value()) {
			  interaction &interaction = interactions[cell.interaction_handle.value()];
			  if (interaction.kind == interaction_kind::LIGHT_MANUAL || interaction.kind == interaction_kind::HEAVY_MANUAL) {
				  vec2 cell_center{activate_req.column, activate_req.line};
				  if (hitbox.center.to(cell_center).norm() <= components.properties[handle].activate_range) {
					  state.preparing_action   = {activate_req};
					  state.ticks_before_ready = this->activators[interaction.activator_handle].activation_difficulty;
				  }
			  }
		  }
	  },
	  [&](ninja_api::nnj_attack_request &attack_req) {
		  auto &health = components.health;
		  if (health[attack_req.target_handle] && components.hitbox[attack_req.target_handle].has_value()) {
			  component::hitbox &target_hitbox = components.hitbox[attack_req.target_handle].value();
			  if (hitbox.center.to(target_hitbox.center).norm() <= components.properties[handle].attack_range) {
				  state.preparing_action   = {attack_req};
				  state.ticks_before_ready = components.properties[handle].attack_delay;
			  }
		  }
	  },
	  [&](ninja_api::nnj_throw_request &throw_req) {
		  state.preparing_action   = {throw_req};
		  state.ticks_before_ready = components.properties[handle].throw_delay;
	  }};

	std::visit(decision_visitor, *components.decision[handle]);

	components.decision[handle] = {};

	adapter.mark_entity_as_dirty(handle);
}

void model::world::action_system(adapter::adapter &adapter, handle_t handle) {
	ENSURE_COMPONENT(handle, hitbox);

	component::state &state = components.state[handle];

	if (!state.preparing_action.has_value()) {
		return;
	}

	if (state.ticks_before_ready > 0) {
		state.ticks_before_ready -= 1;
		return;
	}

	component::hitbox &hitbox         = components.hitbox[handle].value();
	component::properties &properties = components.properties[handle];

	utils::visitor action_visitor{
	  [&](ninja_api::nnj_activate_request &activate_req) {
		  const cell &cell = map[activate_req.column][activate_req.line];
		  if (cell.interaction_handle.has_value()) {
			  interaction &interaction = interactions[cell.interaction_handle.value()];
			  if (interaction.kind == interaction_kind::LIGHT_MANUAL || interaction.kind == interaction_kind::HEAVY_MANUAL) {
				  vec2 cell_center{activate_req.column, activate_req.line};
				  if (hitbox.center.to(cell_center).norm() <= components.properties[handle].activate_range) {
					  fire_activator(adapter, interaction.activator_handle, event_reason::ENTITY);
				  }
			  }
		  }
	  },
	  [&](ninja_api::nnj_attack_request &attack_req) {
		  auto &health = components.health;
		  if (health[attack_req.target_handle] && components.hitbox[attack_req.target_handle]) {
			  component::hitbox &target_hitbox = *components.hitbox[attack_req.target_handle];
			  if (hitbox.center.to(target_hitbox.center).norm() <= components.properties[handle].attack_range) {
				  health[attack_req.target_handle]->points -= 1;
				  if (health[attack_req.target_handle]->points == 0) {
					  reset_entity(attack_req.target_handle);
					  adapter.hide_entity(
					    adapter::model_handle{attack_req.target_handle, adapter::model_handle::ENTITY});
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

	adapter.mark_entity_as_dirty(handle);
}

void model::world::movement_system(adapter::adapter &adapter, handle_t handle) {
	ENSURE_COMPONENT(handle, movement);
	ENSURE_COMPONENT(handle, hitbox);

	component::movement &movement        = components.movement[handle].value();
	component::hitbox &hitbox            = components.hitbox[handle].value();
	component::properties &properties    = components.properties[handle];
	component::metadata &entity_metadata = components.metadata[handle];

	float rotation = std::clamp(movement.rotation, -properties.rotation_speed, properties.rotation_speed);
	if (std::abs(rotation) > SIGNIFICANT_ROTATION_EPSILON) {
		rotate_entity(adapter, handle, rotation);
	}

	float dx = std::cos(hitbox.rad) * movement.forward_diff + std::cos(hitbox.rad + uni::math::pi_2<float>) * movement.lateral_diff;
	float dy = -(std::sin(hitbox.rad) * movement.forward_diff + std::sin(hitbox.rad + uni::math::pi_2<float>) * movement.lateral_diff);
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
		move_entity(adapter, handle, displacement);
	}

	components.movement[handle] = {};

	adapter.mark_entity_as_dirty(handle);

	// Check for victory
	if (entity_metadata.kind == ninja_api::nnj_entity_kind::EK_DLL) {
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

// -- Physics helpers -- //

float slowdown_factor(float r) {
	return -(0.3 * r * r) / 2 - 0.1 * std::pow(std::cos(1.1 * r), 2) + 1.1 - 0.03 * r * r * std::cos(r); // NOLINT
}

void model::world::move_entity(adapter::adapter &adapter, handle_t handle, vec2 movement) {
	component::hitbox &hitbox = *components.hitbox[handle];

	float old_x = hitbox.center.x;
	hitbox.center.x += movement.x;
	if (entity_check_collision(handle)) {
		hitbox.center.x = old_x;
	}

	float old_y = hitbox.center.y;
	hitbox.center.y += movement.y;
	if (entity_check_collision(handle)) {
		hitbox.center.y = old_y;
	}

	adapter.move_entity(adapter::model_handle{handle, adapter::model_handle::ENTITY}, hitbox.center.x, hitbox.center.y);
}

void model::world::rotate_entity(adapter::adapter &adapter, handle_t handle, float rotation_rad) {
	ENSURE_COMPONENT(handle, hitbox);
	component::hitbox &hitbox = components.hitbox[handle].value();

	float old_rad = hitbox.rad;

	hitbox.rad += rotation_rad;
	if (hitbox.rad >= uni::math::pi<float>) {
		hitbox.rad -= 2 * uni::math::pi<float>;
	} else if (hitbox.rad <= -uni::math::pi<float>) {
		hitbox.rad += 2 * uni::math::pi<float>;
	}

	if (entity_check_collision(handle)) {
		hitbox.rad = old_rad;
	} else {
		adapter.rotate_entity(adapter::model_handle{handle, adapter::model_handle::ENTITY}, hitbox.rad);
	}
}

bool model::world::entity_check_collision(handle_t handle) {
	ENSURE_COMPONENT_OR_ELSE(handle, hitbox, { return false; });
	component::hitbox &hitbox = components.hitbox[handle].value();

	obb this_box{hitbox};

	// Against grid cells
	bounding_circle this_circle{hitbox};
	for (const cell_view &cell : map.subgrid(this_box)) {
		if (cell.type != cell_type::GROUND) {
			aabb cell_box{cell.pos};
			if (circle_aabb_test(this_circle, cell_box)) {
				return true;
			}
		}
	}

	// Against other entities
	for (handle_t other_handle = cst::max_entities; (other_handle--) != 0u;) {
		if (other_handle != handle) {
			ENSURE_COMPONENT_OR_ELSE(other_handle, hitbox, { continue; });
			component::hitbox &other_hitbox = components.hitbox[other_handle].value();

			obb other_box{other_hitbox};

			if (obb_obb_sat_test(this_box, other_box)) {
				return true;
			}
		}
	}

	return false;
}

// -- Interactions helpers -- //

void model::world::fire_activator(adapter::adapter &adapter, handle_t handle, event_reason reason) {
	auto &activator = activators[handle];

	if (activator.enabled && reason == event_reason::ENTITY) {
		// Disable activator if it is enabled and an entity activate it again.
		activator.enabled = false;

		// Remove future events for this activator from the queue.
		m_event_queue.clear_for_handle(handle);
	} else if (activator.activation_delay != 0 && reason != event_reason::DELAY) {
		// If an activation delay is set for this activator, enqueue an event fire firing the actionnable later.
		// When the activator is fired because of event_reason::DELAY, a new event should not be enqueued again.
		m_event_queue.add_event(handle, activator.activation_delay, event_reason::DELAY);
		activator.enabled = true;
	} else {
		// Otherwise, the actionnables should be actually actionned.
		for (handle_t target : activators[handle].targets) {
			fire_actionable(adapter, target);
		}

		// Notify that the activator actionned the actionnables.
		adapter.fire_activator(adapter::model_handle{handle, adapter::model_handle::ACTIVATOR});

		// Job is done, mark the activator as disabled.
		activator.enabled = false;

		// … but reenable it if there is refire_after / refire_repeat configured on it.
		if (activator.refire_after && (reason != event_reason::REFIRE || activator.refire_repeat)) {
			m_event_queue.add_event(handle, *activators[handle].refire_after, event_reason::REFIRE);
			activator.enabled = true;
		}
	}
}

void model::world::fire_actionable(adapter::adapter &adapter, handle_t handle) {
	actionables[handle].make_action({*this, adapter});
}
