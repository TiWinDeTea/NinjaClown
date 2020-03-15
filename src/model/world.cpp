#include <array>
#include <cmath>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/collision.hpp"
#include "model/world.hpp"

const float PI = 3.14159f;

void model::world::update(adapter::adapter &adapter) {
	for (size_t handle = max_entities; handle--;) {
		single_entity_simple_update(adapter, handle);
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
				adapter.rotate_entity(adapter::model_handle{handle}, angle);
				break;
			case component::decision::TURN_RIGHT:
				// TODO: check collision
				components.hitbox[handle]->rad -= components.properties[handle].rotation_speed;
				if (angle <= -PI) {
					angle += 2 * PI;
				}
				adapter.rotate_entity(adapter::model_handle{handle}, angle);
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
				for (cell &c : grid.radius(hitbox.center.x, hitbox.center.y, 0.5f)) {
					if (c.interaction_handle) {
						interaction &i = interactions[c.interaction_handle.value()];
						if (i.interactable == interactable_kind::BUTTON) {
							toggle_button(adapter, buttons[i.interactable_handler], grid);
							break;
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
	float old_y = hitbox.center.y;
	hitbox.center.x += dx;
	hitbox.center.y += dy;

	bounding_box box{hitbox};
	for (const cell &c : grid.subgrid(box)) {
		if (c.type != cell_type::GROUND) {
			hitbox.center.x = old_x;
			hitbox.center.y = old_y;
		}
	}

	adapter.move_entity(adapter::model_handle{handle}, hitbox.center.x, hitbox.center.y);
}
