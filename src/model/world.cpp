#include <cmath>
#include <fstream> // ifstream
#include <iostream> // FIXME: tmp
#include <iterator>
#include <spdlog/spdlog.h>
#include <sstream> // ostreamstring
#include <stdexcept> // std::runtime_error

#include "model/world.hpp"
#include "program_state.hpp"

void model::world::update() {
	const float pi            = 3.14159f;
	adapter::adapter &adapter = program_state::global->adapter;

	for (size_t handle = MAX_ENTITIES; handle--;) {
		if (components.decision[handle]) {
			auto &player = program_state::global->viewer.acquire_mobs()->back();
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
					player.set_pos(components.hitbox[handle]->x, components.hitbox[handle]->y);
					adapter.move_entity(adapter::model_handle{handle}, components.hitbox[handle]->x, components.hitbox[handle]->y);
					break;
				case component::decision::MOVE_BACKWARD:
					components.hitbox[handle]->x -= 0.1f * std::cos(components.angle[handle]->rad);
					components.hitbox[handle]->y += 0.1f * std::sin(components.angle[handle]->rad);
					player.set_pos(components.hitbox[handle]->x, components.hitbox[handle]->y);
					adapter.move_entity(adapter::model_handle{handle}, components.hitbox[handle]->x, components.hitbox[handle]->y);
					break;
				case component::decision::ACTIVATE_BUTTON:
					button_system(buttons[0], grid);
					break;
			}
			components.decision[handle] = {};
		}
	}
}
