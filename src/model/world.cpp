#include <cmath>
#include <fstream> // ifstream
#include <iostream> // FIXME: tmp
#include <iterator>
#include <spdlog/spdlog.h>
#include <sstream> // ostreamstring
#include <stdexcept> // std::runtime_error

#include "model/world.hpp"
#include "program_state.hpp"

void model::world::load_map(const std::string &map_path)
{
	std::ifstream fs{map_path};

	size_t width, height;
	fs >> width >> height;

	size_t next_entity_handle  = 0;
	bool ninja_clown_not_found = true;
	grid.resize(height);
	for (size_t row = 0; row < grid.size(); ++row) {
		grid[row].resize(width);
		for (size_t column = 0; column < grid[row].size(); ++column) {
			cell &cell = grid[row][column];

			char type;
			fs >> std::noskipws >> type;
			if (type == '\n') {
				fs >> std::noskipws >> type;
			}

			switch (type) {
				case '#':
				case 'P':
					cell.type = cell_type::WALL;
					break;
				case 'b':
					cell.type               = cell_type::GROUND;
					cell.interaction_handle = {interactions.size()};
					interactions.push_back(interaction{interaction_kind::LIGHT_MANUAL, interactable_kind::BUTTON, buttons.size()});
					buttons.push_back(button{2, 4}); // FIXME
					break;
				case '@': {
					ninja_clown_not_found                 = false;
					ninja_clown_handle                    = next_entity_handle++;
					components.health[ninja_clown_handle] = {1};
					components.hitbox[ninja_clown_handle] = {static_cast<float>(column), static_cast<float>(row), 1.f, 1.f};
					components.angle[ninja_clown_handle]  = {0.f};
				}
				case 'D':
				case ' ':
					cell.type = cell_type::GROUND;
					break;
				default:
					cell.type = cell_type::CHASM;
					break;
			}
		}
	}

	if (ninja_clown_not_found) {
		spdlog::error("ninja clown not found in the map file!");
		throw std::runtime_error{"ninja clown not found"};
	}
}

void model::world::update()
{
	for (size_t handle = MAX_ENTITIES; handle--;) {
		if (components.decision[handle]) {
			switch (*components.decision[handle]) {
				case component::decision::TURN_LEFT:
					components.angle[handle]->rad += 0.1f;
					break;
				case component::decision::TURN_RIGHT:
					components.angle[handle]->rad -= 0.1f;
					break;
				case component::decision::MOVE_FORWARD:
					components.hitbox[handle]->x += std::cos(components.angle[handle]->rad);
					components.hitbox[handle]->y -= std::sin(components.angle[handle]->rad);
					break;
				case component::decision::MOVE_BACKWARD:
					components.hitbox[handle]->x -= std::cos(components.angle[handle]->rad);
					components.hitbox[handle]->y += std::sin(components.angle[handle]->rad);
					break;
				case component::decision::ACTIVATE_BUTTON:
					button_system(buttons[0], grid);
					break;
			}
		}
	}

	std::ostringstream oss;
	for (auto &row : grid) {
		oss << "\n";
		for (auto &cell : row) {
			switch (cell.type) {
				case cell_type::CHASM:
					oss << "X";
					break;
				case cell_type::GROUND:
					oss << " ";
					break;
				case cell_type::WALL:
					oss << "#";
					break;
			}
		}
	}
	spdlog::info("{}", oss.str());
	spdlog::info("ninja clown at ({}, {}) with angle {} rad", components.hitbox[ninja_clown_handle]->center_x(),
	             components.hitbox[ninja_clown_handle]->center_y(), components.angle[ninja_clown_handle]->rad);
}
