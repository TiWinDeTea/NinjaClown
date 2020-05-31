#include <array>
#include <cmath>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/collision.hpp"
#include "model/world.hpp"

const float PI = 3.14159f;

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
		components.metadata[i]   = {};
		components.properties[i] = {};
		components.decision[i]   = ninja_api::nnj_decision{ninja_api::DK_NONE};
		components.health[i].reset();
		components.hitbox[i].reset();
	}
}

void model::world::single_entity_simple_update(adapter::adapter &adapter, size_t handle) {
	ninja_api::nnj_decision &decision = components.decision[handle];
	component::properties &properties = components.properties[handle];

	switch (decision.kind) {
		case ninja_api::DK_NONE:
			// nothing to do here
			break;
		case ninja_api::DK_MOVEMENT: {
			float rotation = std::clamp(decision.movement.rotation, -properties.rotation_speed, properties.rotation_speed);
			if (std::abs(rotation) > 0.001) { // FIXME: eta variable
				rotate_entity(adapter, handle, rotation);
			}

			// FIXME: check for NaN in user input
			float dx = std::cos(components.hitbox[handle]->rad) * decision.movement.forward_diff
			           + std::cos(components.hitbox[handle]->rad + uni::math::pi_2<float>) * decision.movement.lateral_diff;
			float dy = -(std::sin(components.hitbox[handle]->rad) * decision.movement.forward_diff
			             + std::sin(components.hitbox[handle]->rad + uni::math::pi_2<float>) * decision.movement.lateral_diff);
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
				if (distance < components.hitbox[handle]->half_height() && distance < components.hitbox[handle]->half_width()) {
					spdlog::info("You win."); // TODO
				}
			}
			break;
		}
		case ninja_api::DK_ACTIVATE: {
			component::hitbox &hitbox = *components.hitbox[handle];
			const cell &c             = grid[decision.activate.column][decision.activate.line];
			if (c.interaction_handle) {
				interaction &i = interactions[*c.interaction_handle];
				if (i.kind == interaction_kind::LIGHT_MANUAL || i.kind == interaction_kind::HEAVY_MANUAL) {
					vec2 cell_center{decision.activate.column, decision.activate.line};
					if (hitbox.center.to(cell_center).norm() <= components.properties[handle].activate_range) {
						fire_activator(adapter, i.interactable_handler);
					}
				}
			}
			break;
		}
		default:
			spdlog::warn("decision kind not yet implemented");
			break;
	}

	components.decision[handle].kind = ninja_api::DK_NONE;
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
	if (hitbox.rad >= PI) {
		hitbox.rad -= 2 * PI;
	}
	else if (hitbox.rad <= -PI) {
		hitbox.rad += 2 * PI;
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
