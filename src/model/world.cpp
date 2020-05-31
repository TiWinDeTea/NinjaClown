#include <array>
#include <cmath>
#include <spdlog/spdlog.h>
#include <variant>

#include "adapter/adapter.hpp"
#include "model/collision.hpp"
#include "model/world.hpp"
#include "utils/visitor.hpp"

const float significant_rotation_eta = 0.001f;

void model::world::update(adapter::adapter &adapter) {
	for (size_t handle = cst::max_entities; handle--;) {
		single_entity_simple_update(adapter, handle);
	}
}

void model::world::reset() {
	grid.resize(0, 0);
	interactions.clear();
	activators.clear();
	actionables.clear();

	for (unsigned int i = 0; i < cst::max_entities; ++i) {
		reset_entity(i);
	}
}

void model::world::reset_entity(size_t handle) {
	components.metadata[handle]   = {};
	components.properties[handle] = {};
	components.decision[handle].reset();
	components.health[handle].reset();
	components.hitbox[handle].reset();
}

void model::world::single_entity_simple_update(adapter::adapter &adapter, size_t handle) {
	if (!components.decision[handle] || !components.hitbox[handle]) {
		return;
	}

	component::decision &decision     = *components.decision[handle];
	component::hitbox &hitbox         = *components.hitbox[handle];
	component::properties &properties = components.properties[handle];

	utils::visitor visitor{
	  [&](ninja_api::nnj_movement_request &mov_req) {
		  float rotation = std::clamp(mov_req.rotation, -properties.rotation_speed, properties.rotation_speed);
		  if (std::abs(rotation) > significant_rotation_eta) {
			  rotate_entity(adapter, handle, rotation);
		  }

		  float dx = std::cos(components.hitbox[handle]->rad) * mov_req.forward_diff
		             + std::cos(components.hitbox[handle]->rad + uni::math::pi_2<float>) * mov_req.lateral_diff;
		  float dy = -(std::sin(components.hitbox[handle]->rad) * mov_req.forward_diff
		               + std::sin(components.hitbox[handle]->rad + uni::math::pi_2<float>) * mov_req.lateral_diff);
		  vec2 movement{dx, dy};
		  if (movement.norm() > properties.move_speed) {
			  // cap movement vector to max speed
			  movement.unitify();
			  movement.x *= properties.move_speed;
			  movement.y *= properties.move_speed;
		  }
		  if (movement.norm() != 0) {
			  move_entity(adapter, handle, movement);
		  }

		  if (handle == ninja_clown_handle) {
			  float distance = components.hitbox[handle]
			                     ->center.to({target_tile.x + cst::cell_width / 2.f, target_tile.y + cst::cell_height / 2.f})
			                     .norm();
			  if (distance < components.hitbox[handle]->height() && distance < components.hitbox[handle]->width()) {
				  spdlog::info("You win."); // TODO
			  }
		  }
	  },
	  [&](ninja_api::nnj_activate_request &activate_req) {
		  const cell &c = grid[activate_req.column][activate_req.line];
		  if (c.interaction_handle) {
			  interaction &i = interactions[*c.interaction_handle];
			  if (i.kind == interaction_kind::LIGHT_MANUAL || i.kind == interaction_kind::HEAVY_MANUAL) {
				  vec2 cell_center{activate_req.column, activate_req.line};
				  if (hitbox.center.to(cell_center).norm() <= components.properties[handle].activate_range) {
					  fire_activator(adapter, i.interactable_handler);
				  }
			  }
		  }
	  },
	  [&](ninja_api::nnj_attack_request &attack_req) {
		  if (components.health[attack_req.target_handle] && components.hitbox[attack_req.target_handle]) {
			  component::hitbox &target_hitbox = *components.hitbox[attack_req.target_handle];
			  if (hitbox.center.to(target_hitbox.center).norm() <= components.properties[handle].attack_range) {
				  components.health[attack_req.target_handle]->points -= 1;
				  if (components.health[attack_req.target_handle]->points == 0) {
					  reset_entity(attack_req.target_handle);
					  adapter.hide_entity(adapter::model_handle{attack_req.target_handle, adapter::model_handle::ENTITY});
				  }
			  }
		  }
	  },
	  [&](ninja_api::nnj_throw_request &throw_req) {

	  }};

	std::visit(visitor, decision);
	components.decision[handle] = {};
}

void model::world::move_entity(adapter::adapter &adapter, size_t handle, vec2 movement) {
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

void model::world::rotate_entity(adapter::adapter &adapter, size_t handle, float rotation_rad) {
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

bool model::world::entity_check_collision(size_t handle) {
	obb box{*components.hitbox[handle]};

	// with map
	bounding_circle circle{*components.hitbox[handle]};
	for (const cell_view &c : grid.subgrid(box)) {
		if (c.type != cell_type::GROUND) {
			aabb cell_box{c.pos};
			if (circle_aabb_test(circle, cell_box)) {
				return true;
			}
		}
	}

	// other entities
	for (size_t other_handle = cst::max_entities; other_handle--;) {
		if (other_handle != handle && components.hitbox[other_handle]) {
			obb other{*components.hitbox[other_handle]};
			if (obb_obb_sat_test(box, other)) {
				return true;
			}
		}
	}

	return false;
}

void model::world::fire_activator(adapter::adapter &adapter, size_t handle) {
	for (size_t target : activators[handle].targets) {
		fire_actionable(adapter, target);
	}
	adapter.fire_activator(adapter::model_handle{handle, adapter::model_handle::ACTIVATOR});
}

void model::world::fire_actionable(adapter::adapter &adapter, size_t handle) {
	actionables[handle].make_action({*this, adapter});
}
