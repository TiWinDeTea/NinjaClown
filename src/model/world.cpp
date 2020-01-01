#include <fstream>
#include <iostream> // FIXME: tmp
#include <iterator>
#include <sstream>

#include "model/world.hpp"

model::world::world(size_t width, size_t height) noexcept
{
	m_grid.resize(height);
	for (auto &row : m_grid) {
		row.resize(width);
	}
}

model::world::world(const std::string &map_path)
{
	std::ifstream fs{map_path};

	size_t width, height;
	fs >> width >> height;

	m_grid.resize(height);
	for (auto &row : m_grid) {
		row.resize(width);
		for (auto &cell : row) {
			char type;
			fs >> type;
			switch (type) {
				case '#':
				case 'P':
					cell.type = cell_type::WALL;
					break;
				case 'b':
					cell.type               = cell_type::GROUND;
					cell.interaction_handle = {m_interactions.size()};
					m_interactions.push_back(interaction{interaction_kind::LIGHT_MANUAL, interactable_kind::BUTTON, m_buttons.size()});
					m_buttons.push_back(button{1, 2}); // FIXME
					break;
				case 'D':
				case 'g':
					cell.type = cell_type::GROUND;
					break;
				default:
					cell.type = cell_type::VOID;
					break;
			}
		}
	}
}

void model::world::update()
{
	std::cout << m_grid.size() << " " << m_grid[0].size() << "\n";

	for (auto &row : m_grid) {
		for (auto &cell : row) {
			switch (cell.type) {
				case cell_type::VOID:
					std::cout << "X";
					break;
				case cell_type::GROUND:
					std::cout << " ";
					break;
				case cell_type::WALL:
					std::cout << "#";
					break;
			}
		}
		std::cout << "\n";
	}
}
