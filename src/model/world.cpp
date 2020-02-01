#include <array>
#include <cmath>
#include <spdlog/spdlog.h>

#include "adapter/adapter.hpp"
#include "model/world.hpp"

void model::world::update(adapter::adapter &adapter) {
	const float pi = 3.14159f;

	for (size_t handle = MAX_ENTITIES; handle--;) {
		if (components.decision[handle]) {
			float &angle = components.angle[handle]->rad;
			switch (*components.decision[handle]) {
				case component::decision::TURN_LEFT:
					angle += 0.1f;
					if (angle >= pi) {
						angle -= 2 * pi;
					}
					adapter.rotate_entity(adapter::model_handle{handle}, angle);
					break;
				case component::decision::TURN_RIGHT:
					components.angle[handle]->rad -= 0.1f;
					if (angle <= -pi) {
						angle += 2 * pi;
					}
					adapter.rotate_entity(adapter::model_handle{handle}, angle);
					break;
				case component::decision::MOVE_FORWARD:
					components.hitbox[handle]->x += 0.1f * std::cos(components.angle[handle]->rad);
					components.hitbox[handle]->y -= 0.1f * std::sin(components.angle[handle]->rad);
					adapter.move_entity(adapter::model_handle{handle}, components.hitbox[handle]->x, components.hitbox[handle]->y);
					break;
				case component::decision::MOVE_BACKWARD:
					components.hitbox[handle]->x -= 0.1f * std::cos(components.angle[handle]->rad);
					components.hitbox[handle]->y += 0.1f * std::sin(components.angle[handle]->rad);
					adapter.move_entity(adapter::model_handle{handle}, components.hitbox[handle]->x, components.hitbox[handle]->y);
					break;
				case component::decision::ACTIVATE_BUTTON:
					const float center_x = components.hitbox[handle]->center_x();
					const float center_y = components.hitbox[handle]->center_y();
					bool done            = false;
					for (float diff_x : std::array{-0.5f, 0.f, 0.5f}) {
						for (float diff_y : std::array{-0.5f, 0.f, 0.5f}) {
							const auto tile_x = static_cast<size_t>(center_x + diff_x);
							const auto tile_y = static_cast<size_t>(center_y + diff_y);
							if (grid[tile_x][tile_y].interaction_handle) {
								interaction &i = interactions[grid[tile_x][tile_y].interaction_handle.value()];
								if (i.interactable == interactable_kind::BUTTON) {
									toggle_button(adapter, buttons[i.interactable_handler], grid);
									done = true;
									break;
								}
							}
						}
						if (done) {
							break;
						}
					}
					break;
			}
			components.decision[handle] = {};
		}
	}
}
