#include <array>
#include <cmath>
#include <model/event.hpp>
#include <spdlog/spdlog.h>
#include <variant>

#include "adapter/adapter.hpp"
#include "model/collision.hpp"
#include "model/world.hpp"
#include "utils/visitor.hpp"

const float SIGNIFICANT_ROTATION_EPSILON = 0.001f;

float slowdown_factor(float r) {
	return -(0.3 * r * r) / 2 - 0.1 * std::pow(std::cos(1.1 * r), 2) + 1.1 - 0.03 * r * r * std::cos(r); // NOLINT
}

void model::world::update(adapter::adapter &adapter) {
	for (handle_t handle = cst::max_entities; (handle--) != 0u;) {
		single_entity_simple_update(adapter, handle);
	}

	m_event_queue.update(*this, adapter);
}

void model::world::reset() {
	map.clear();
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
	components.decision[handle].reset();
	components.health[handle].reset();
	components.hitbox[handle].reset();
}

void model::world::single_entity_simple_update(adapter::adapter &adapter, handle_t handle) {
	single_entity_decision_update(adapter, handle);
	single_entity_action_update(adapter, handle);
}

void model::world::single_entity_action_update(adapter::adapter &adapter, handle_t handle) {
	if (!components.state[handle].preparing_action || !components.hitbox[handle]) {
		return;
	}

	component::hitbox &hitbox         = *components.hitbox[handle];
	component::state &state           = components.state[handle];
	component::properties &properties = components.properties[handle];

	if (state.ticks_before_ready > 0) {
		state.ticks_before_ready -= 1;
		return;
	}

	utils::visitor visitor{
	  [&](ninja_api::nnj_activate_request &activate_req) {
		  const cell &cell = map[activate_req.column][activate_req.line];
		  if (cell.interaction_handle) {
			  interaction &interaction = interactions[*cell.interaction_handle];
			  if (interaction.kind == interaction_kind::LIGHT_MANUAL || interaction.kind == interaction_kind::HEAVY_MANUAL) {
				  vec2 cell_center{activate_req.column, activate_req.line};
				  if (hitbox.center.to(cell_center).norm() <= components.properties[handle].activate_range) {
					  fire_activator(adapter, interaction.interactable_handler, event_reason::NONE);
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
					    adapter::model_handle{static_cast<handle_t>(attack_req.target_handle), adapter::model_handle::ENTITY});
				  }
			  }
		  }
	  },
	  [&](ninja_api::nnj_throw_request &throw_req) {

	  },
	};

	std::visit(visitor, *state.preparing_action);
	state.preparing_action = {};
	adapter.mark_entity_as_dirty(handle);
}

void model::world::single_entity_decision_update(adapter::adapter &adapter, handle_t handle) {
	if (!components.decision[handle] || !components.hitbox[handle]) {
		return;
	}

	component::decision &decision     = *components.decision[handle];
	component::hitbox &hitbox         = *components.hitbox[handle];
	component::properties &properties = components.properties[handle];
	component::state &state           = components.state[handle];

	// Cancel action currently being prepared
	state.preparing_action   = {};
	state.ticks_before_ready = 0;

	utils::visitor visitor_with_a_very_long_name_for_clang_format{
	  [&](ninja_api::nnj_movement_request &mov_req) {
		  float rotation = std::clamp(mov_req.rotation, -properties.rotation_speed, properties.rotation_speed);
		  if (std::abs(rotation) > SIGNIFICANT_ROTATION_EPSILON) {
			  rotate_entity(adapter, handle, rotation);
		  }

		  float dx = std::cos(components.hitbox[handle]->rad) * mov_req.forward_diff
		             + std::cos(components.hitbox[handle]->rad + uni::math::pi_2<float>) * mov_req.lateral_diff;
		  float dy = -(std::sin(components.hitbox[handle]->rad) * mov_req.forward_diff
		               + std::sin(components.hitbox[handle]->rad + uni::math::pi_2<float>) * mov_req.lateral_diff);
		  vec2 movement{dx, dy};

		  if (movement.norm() != 0) {
              // maximal speed is achieved by fully moving forward, otherwise entity is slowed down
              float max_norm = properties.move_speed * slowdown_factor(components.hitbox[handle]->rad + movement.atan2());
			  if (movement.norm() > max_norm) {
				  // cap movement vector to max speed
				  movement.unitify();
				  movement.x *= max_norm;
				  movement.y *= max_norm;
			  }
			  move_entity(adapter, handle, movement);
		  }

		  const auto &meta = components.metadata[handle]; // MSVC hax, won’t compile when inlining this variable
		  if (meta.kind == ninja_api::nnj_entity_kind::EK_DLL) {
			  float distance = components.hitbox[handle]
			                     ->center.to({target_tile.x + cst::cell_width / 2.f, target_tile.y + cst::cell_height / 2.f})
			                     .norm();
			  if (distance < components.hitbox[handle]->height() && distance < components.hitbox[handle]->width()) {
				  spdlog::info("You win."); // TODO

			  }
		  }
	  },
	  [&](ninja_api::nnj_activate_request &activate_req) {
		  const cell &cell = map[activate_req.column][activate_req.line];
		  if (cell.interaction_handle) {
			  interaction &interaction = interactions[*cell.interaction_handle];
			  if (interaction.kind == interaction_kind::LIGHT_MANUAL || interaction.kind == interaction_kind::HEAVY_MANUAL) {
				  vec2 cell_center{activate_req.column, activate_req.line};
				  if (hitbox.center.to(cell_center).norm() <= components.properties[handle].activate_range) {
					  state.preparing_action   = {activate_req};
					  state.ticks_before_ready = this->activators[interaction.interactable_handler].activation_difficulty;
				  }
			  }
		  }
	  },
	  [&](ninja_api::nnj_attack_request &attack_req) {
		  auto &health = components.health;
		  if (health[attack_req.target_handle] && components.hitbox[attack_req.target_handle]) {
			  component::hitbox &target_hitbox = *components.hitbox[attack_req.target_handle];
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

	std::visit(visitor_with_a_very_long_name_for_clang_format, *components.decision[handle]);
	components.decision[handle] = {};
	adapter.mark_entity_as_dirty(handle);
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
	component::hitbox &hitbox = components.hitbox[handle].value();
	float old_rad             = hitbox.rad;

	hitbox.rad += rotation_rad;
	if (hitbox.rad >= uni::math::pi<float>) {
		hitbox.rad -= 2 * uni::math::pi<float>;
	}
	else if (hitbox.rad <= -uni::math::pi<float>) {
		hitbox.rad += 2 * uni::math::pi<float>;
	}

	if (entity_check_collision(handle)) {
		hitbox.rad = old_rad;
	}
	else {
		adapter.rotate_entity(adapter::model_handle{handle, adapter::model_handle::ENTITY}, hitbox.rad);
	}
}

bool model::world::entity_check_collision(handle_t handle) {
	obb box{*components.hitbox[handle]};

	// with map
	bounding_circle circle{*components.hitbox[handle]};
	for (const cell_view &c : map.subgrid(box)) {
		if (c.type != cell_type::GROUND) {
			aabb cell_box{c.pos};
			if (circle_aabb_test(circle, cell_box)) {
				return true;
			}
		}
	}

	// other entities
	for (handle_t other_handle = cst::max_entities; (other_handle--) != 0u;) {
		if (other_handle != handle && components.hitbox[other_handle]) {
			obb other{*components.hitbox[other_handle]};
			if (obb_obb_sat_test(box, other)) {
				return true;
			}
		}
	}

	return false;
}

void model::world::fire_activator(adapter::adapter &adapter, handle_t handle, event_reason reason) {
	auto &activator = activators[handle];

	m_event_queue.clear_for_handle(handle);

	if (activator.enabled && reason == event_reason::NONE) {
		activator.enabled = false;
	}
	else {
		if (activator.activation_delay != 0 && reason != event_reason::DELAY) {
			m_event_queue.add_event(handle, activator.activation_delay, event_reason::DELAY);
			activator.enabled = true;
		} else {
			activator.enabled = false;
			for (handle_t target : activators[handle].targets) {
				fire_actionable(adapter, target);
			}
			adapter.fire_activator(adapter::model_handle{handle, adapter::model_handle::ACTIVATOR});

			if (activator.refire_after && (reason != event_reason::REFIRE || activator.refire_repeat)) {
				m_event_queue.add_event(handle, *activators[handle].refire_after, event_reason::REFIRE);
				activator.enabled = true;
			}
		}
	}
}

void model::world::fire_actionable(adapter::adapter &adapter, handle_t handle) {
	actionables[handle].make_action({*this, adapter});
}
