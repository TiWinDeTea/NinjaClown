#include <array>
#include <cmath>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/world.hpp"

const float PI = 3.14159f;

void model::world::update(adapter::adapter &adapter) {
	for (size_t handle = MAX_ENTITIES; handle--;) {
		single_entity_simple_update(adapter, handle);
	}
}

void model::world::single_entity_simple_update(adapter::adapter& adapter, size_t handle){
	if (components.decision[handle]) {
		float &angle = components.angle[handle]->rad;
		switch (*components.decision[handle]) {
			case component::decision::TURN_LEFT:
				angle += components.properties[handle].rotation_speed;
				if (angle >= PI) {
					angle -= 2 * PI;
				}
				adapter.rotate_entity(adapter::model_handle{handle}, angle);
				break;
			case component::decision::TURN_RIGHT:
				components.angle[handle]->rad -= components.properties[handle].rotation_speed;
				if (angle <= -PI) {
					angle += 2 * PI;
				}
				adapter.rotate_entity(adapter::model_handle{handle}, angle);
				break;
			case component::decision::MOVE_FORWARD:
				move_entity(
					adapter, handle,
					+components.properties[handle].move_speed * std::cos(components.angle[handle]->rad),
					-components.properties[handle].move_speed * std::sin(components.angle[handle]->rad)
				);
				break;
			case component::decision::MOVE_BACKWARD:
				move_entity(
					adapter, handle,
					-components.properties[handle].move_speed * std::cos(components.angle[handle]->rad),
					+components.properties[handle].move_speed * std::sin(components.angle[handle]->rad)
				);
				break;
			case component::decision::ACTIVATE_BUTTON:
				component::hitbox& hitbox = components.hitbox[handle].value();
				for (cell &c : grid.radius(hitbox.center_x(), hitbox.center_y(), 0.5f)) {
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
	const float old_x = components.hitbox[handle]->x;
	const float old_y = components.hitbox[handle]->y;
	components.hitbox[handle]->x += dx;
	components.hitbox[handle]->y += dy;

	const component::hitbox& hitbox = components.hitbox[handle].value();
	for (const cell &c : grid.subgrid(
		static_cast<size_t>(hitbox.x),
		static_cast<size_t>(hitbox.y),
		static_cast<size_t>(hitbox.right_x()) + 1,
		static_cast<size_t>(hitbox.bottom_y()) + 1
	)) {
		if (c.type != cell_type::GROUND) {
			components.hitbox[handle]->x = old_x;
			components.hitbox[handle]->y = old_y;
		}
	}

	adapter.move_entity(adapter::model_handle{handle}, components.hitbox[handle]->x, components.hitbox[handle]->y);
}
