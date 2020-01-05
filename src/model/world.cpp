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
				case 'b': {
					cell.type               = cell_type::GROUND;
					cell.interaction_handle = {interactions.size()};
					interactions.push_back(interaction{interaction_kind::LIGHT_MANUAL, interactable_kind::BUTTON, buttons.size()});
					buttons.push_back(button{2, 4}); // FIXME

					view::object o{};
					o.set_pos(static_cast<float>(column), static_cast<float>(row));
					auto obj_anim = program_state::global->resource_manager.object_animation(utils::resource_manager::object_id::button);
					assert(obj_anim);
					o.set_animation(*obj_anim);
                    program_state::global->viewer.acquire_objects()->push_back(o);
					break;
				}
				case '@': {
					ninja_clown_not_found                 = false;
					ninja_clown_handle                    = next_entity_handle++;
					components.health[ninja_clown_handle] = {1};
					components.hitbox[ninja_clown_handle] = {static_cast<float>(column), static_cast<float>(row), 1.f, 1.f};
					components.angle[ninja_clown_handle]  = {0.f};

					view::mob m{};
					m.set_animations(program_state::global->resource_manager.mob_animations(utils::resource_manager::mob_id::player).value());
					m.set_direction(view::facing_direction::E);
					m.set_pos(static_cast<float>(column), static_cast<float>(row));
                    program_state::global->viewer.acquire_mobs()->emplace_back(m);
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

	// FIXME: use adapter anyway
	std::vector<std::vector<view::map::cell>> view_map;
	view_map.resize(width);
	for (unsigned int x = 0 ; x < width ; ++x) {
	    view_map[x].resize(height);
	    for (unsigned int y = 0 ; y < height ; ++y) {
	        switch (grid[y][x].type) {
	            case cell_type::CHASM:
	                [[fallthrough]];
	            case cell_type::WALL :
	                view_map[x][y] = view::map::cell::abyss;
	                break;
	            case cell_type::GROUND:
	                view_map[x][y] = view::map::cell::concrete_tile;
	        }
	    }
	}
	program_state::global->viewer.update_map(std::move(view_map));
}

void model::world::update()
{
    const float pi = 3.14159f;

	for (size_t handle = MAX_ENTITIES; handle--;) {
		if (components.decision[handle]) {
            auto& player = program_state::global->viewer.acquire_mobs()->back();
            float& angle = components.angle[handle]->rad;
			switch (*components.decision[handle]) {
				case component::decision::TURN_LEFT:
					 angle += 0.1f;
					if (angle >= pi) {
					    angle -= 2*pi;
					}
					break;
				case component::decision::TURN_RIGHT:
					components.angle[handle]->rad -= 0.1f;
                    if (angle <= -pi) {
                        angle += 2*pi;
                    }
					break;
				case component::decision::MOVE_FORWARD:
					components.hitbox[handle]->x += 0.1f * std::cos(components.angle[handle]->rad);
					components.hitbox[handle]->y -= 0.1f * std::sin(components.angle[handle]->rad);
					player.set_pos(components.hitbox[handle]->x, components.hitbox[handle]->y);
					break;
				case component::decision::MOVE_BACKWARD:
					components.hitbox[handle]->x -= 0.1f * std::cos(components.angle[handle]->rad);
					components.hitbox[handle]->y += 0.1f * std::sin(components.angle[handle]->rad);
                    player.set_pos(components.hitbox[handle]->x, components.hitbox[handle]->y);
					break;
				case component::decision::ACTIVATE_BUTTON:
					button_system(buttons[0], grid);
					break;
			}

            if (angle >= 3 * pi / 4) {
                player.set_direction(view::facing_direction::W);
            } else if (angle >= pi / 4) {
                player.set_direction(view::facing_direction::N);
            } else if (angle >= -pi / 4) {
                player.set_direction(view::facing_direction::E);
            } else if (angle >= -3 * pi / 4) {
                player.set_direction(view::facing_direction::S);
            } else {
                player.set_direction(view::facing_direction::W);
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
