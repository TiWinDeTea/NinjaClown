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
    grid.resize(0,0);
    interactions.clear();
    activators.clear();
    actionables.clear();

    for (unsigned int i = 0 ; i < cst::max_entities ; ++i) {
        components.metadata[i] = {};
        components.properties[i] = {};
        components.decision[i].reset();
        components.health[i].reset();
        components.hitbox[i].reset();
    }
}


void model::world::single_entity_simple_update(adapter::adapter &adapter, size_t handle) {
	if (components.decision[handle]) {
		float &angle = components.hitbox[handle]->rad;
		switch (*components.decision[handle]) {
			case component::decision::TURN_LEFT:
				// TODO: check collision
				angle += components.properties[handle].rotation_speed;
				if (angle >= PI) {
					angle -= 2 * PI;
				}
				adapter.rotate_entity(adapter::model_handle{handle, adapter::model_handle::ENTITY}, angle);
				break;
			case component::decision::TURN_RIGHT:
				// TODO: check collision
				components.hitbox[handle]->rad -= components.properties[handle].rotation_speed;
				if (angle <= -PI) {
					angle += 2 * PI;
				}
				adapter.rotate_entity(adapter::model_handle{handle, adapter::model_handle::ENTITY}, angle);
				break;
			case component::decision::MOVE_FORWARD:
				move_entity(adapter, handle, +components.properties[handle].move_speed * std::cos(components.hitbox[handle]->rad),
				            -components.properties[handle].move_speed * std::sin(components.hitbox[handle]->rad));
				break;
			case component::decision::MOVE_BACKWARD:
				move_entity(adapter, handle, -components.properties[handle].move_speed * std::cos(components.hitbox[handle]->rad),
				            +components.properties[handle].move_speed * std::sin(components.hitbox[handle]->rad));
				break;
			case component::decision::ACTIVATE_BUTTON:
				component::hitbox &hitbox = components.hitbox[handle].value();
				for (cell_view c : grid.radius(hitbox.center.x, hitbox.center.y, 0.5f)) {
					if (c.interaction_handle) {
						interaction &i = interactions[c.interaction_handle.value()];
						if (i.interactable == interactable_kind::BUTTON) {
							fire_activator(adapter, i.interactable_handler);
						}
					}
				}
		}
		components.decision[handle] = {};
	}
}

void model::world::move_entity(adapter::adapter &adapter, size_t handle, float dx, float dy) {
	component::hitbox &hitbox = components.hitbox[handle].value();

	float old_x = hitbox.center.x;
	hitbox.center.x += dx;
	if (entity_check_collision(hitbox)) {
		hitbox.center.x = old_x;
	}

	float old_y = hitbox.center.y;
	hitbox.center.y += dy;
	if (entity_check_collision(hitbox)) {
		hitbox.center.y = old_y;
	}

	adapter.move_entity(adapter::model_handle{handle, adapter::model_handle::ENTITY}, hitbox.center.x, hitbox.center.y);
}

bool model::world::entity_check_collision(const component::hitbox &hitbox) {
	bounding_box box{hitbox};
	bounding_circle circle{hitbox};
	for (const cell_view &c : grid.subgrid(box)) {
		if (c.type != cell_type::GROUND) {
			bounding_box cell_box{static_cast<float>(c.pos.x), static_cast<float>(c.pos.y), model::cell_width, model::cell_height};
			if (circle_obb_test(circle, cell_box)) {
				return true;
			}
		}
	}

	return false;
}

void model::world::fire_activator(adapter::adapter& adapter, size_t handle) {
    for (size_t target : activators[handle].targets) {
		fire_actionable(adapter, target);
    }
}

void model::world::fire_actionable(adapter::adapter& adapter, size_t handle) {
	actionables[handle].make_action({*this, adapter});
}
