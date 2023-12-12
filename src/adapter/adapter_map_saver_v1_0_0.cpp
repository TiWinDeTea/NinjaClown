#include "adapter/adapter.hpp"
#include "adapter/adapter_mapfile_constants.hpp"
#include "model/model.hpp"
#include "state_holder.hpp"
#include "utils/logging.hpp"
#include "view/game/map.hpp"
#include "view/view.hpp"

#include <fstream>

using fmt::operator""_a;

void adapter::adapter::save_map(std::filesystem::path path) {
	std::ofstream os(path, std::ios_base::trunc);
	if (!os) {
		utils::log::error("adapter.save_map.open_failed", "file"_a = path.generic_string());
		return; // TODO feedback error to user
	}
	boolalpha(os);

	const auto &world = state::access<adapter>::model(m_state).world;
	std::unordered_map<ninja_api::nnj_entity_kind, const char *> behaviour_to_string;
	behaviour_to_string.emplace(std::pair{ninja_api::nnj_entity_kind::EK_NOT_AN_ENTITY, map_file::values::none});
	behaviour_to_string.emplace(std::pair{ninja_api::nnj_entity_kind::EK_HARMLESS, map_file::values::harmless});
	behaviour_to_string.emplace(std::pair{ninja_api::nnj_entity_kind::EK_PATROL, map_file::values::patrol});
	behaviour_to_string.emplace(std::pair{ninja_api::nnj_entity_kind::EK_AGGRESSIVE, map_file::values::aggressive});
	behaviour_to_string.emplace(std::pair{ninja_api::nnj_entity_kind::EK_DLL, map_file::values::dll});

	// Header
	os << "[file]\n"
	      "\tversion = \"1.0.0\"\n"
	      "\n";

	// Mobs
	os << "[mobs]\n";
	for (unsigned int i = 0; i < world.components.metadata.size(); ++i) {
		const auto &comp = world.components;

		if (auto kind = comp.metadata[i].kind; kind == ninja_api::EK_NOT_AN_ENTITY || kind == ninja_api::EK_PROJECTILE) {
			continue;
		}

		const auto &health = comp.health[i]; // extracting variable for linter
		const auto &hitbox = comp.hitbox[i]; // extracting variable for linter
		if (!health || !hitbox) {
			utils::log::warn("adapter.save_map.mob_missing_components", "file"_a = path.generic_string(), "handle"_a = i);
			continue;
		}

		os << "\t[[mobs.spawn]]\n"
		   << "\t\t" << map_file::keys::x_pos << " = " << static_cast<int>(std::round(hitbox->center.x - .5)) << "\n"
		   << "\t\t" << map_file::keys::y_pos << " = " << static_cast<int>(std::round(hitbox->center.y - .5)) << "\n"
		   << "\t\t" << map_file::keys::facing << " = " << hitbox->rad << "\n"
		   << "\t\t" << map_file::keys::hp << " = " << static_cast<int>(health->points) << "\n"
		   << "\t\t" << map_file::keys::behaviour << " = \"" << behaviour_to_string.at(comp.metadata[i].kind) << "\"\n"
		   << "\t\t" << map_file::keys::attack_delay << " = " << comp.properties[i].attack_delay << "\n"
		   << "\t\t" << map_file::keys::throw_delay << " = " << comp.properties[i].throw_delay << "\n"
		   << "\n";
	}

	// Actors
	os << "[actors]\n";

	// Actionables
	for (unsigned int i = 0; i < world.actionables.size(); ++i) {

		using bhvr         = model::actionable::behaviours_ns;
		const auto &actio  = world.actionables[i];
		const auto &behave = actio.behaviour;

		if (behave == bhvr::gate) {
			const auto name = m_view2name.at(m_model2view.at(model_handle{i, model_handle::ACTIONABLE}));
			const bool is_closed{world.map[actio.data.pos.x][actio.data.pos.y].type == model::cell_type::CHASM};

			os << "\t[[actors.spawn]]\n"
			   << "\t\t" << map_file::keys::name << " = \"" << name << "\"\n"
			   << "\t\t" << map_file::keys::type << " = \"" << map_file::values::gate << "\"\n"
			   << "\t\t" << map_file::keys::x_pos << " = " << actio.data.pos.x << "\n"
			   << "\t\t" << map_file::keys::y_pos << " = " << actio.data.pos.y << "\n"
			   << "\t\t" << map_file::keys::closed << " = " << is_closed << "\n"
			   << "\n";
		}

		else if (behave == bhvr::autoshooter) {
			const auto name = m_view2name.at(m_model2view.at(model_handle{i, model_handle::ACTIONABLE}));

			os << "\t[[actors.spawn]]\n"
			   << "\t\t" << map_file::keys::name << " = \"" << name << "\"\n"
			   << "\t\t" << map_file::keys::type << " = \"" << map_file::values::autoshooter << "\"\n"
			   << "\t\t" << map_file::keys::x_pos << " = " << actio.data.pos.x << "\n"
			   << "\t\t" << map_file::keys::y_pos << " = " << actio.data.pos.y << "\n"
			   << "\t\t" << map_file::keys::firing_rate << " = " << actio.data.firing_rate << "\n"
			   << "\t\t" << map_file::keys::facing << " = " << actio.data.angle << "\n"
			   << "\n";
		}

		else if (behave == bhvr::none) {
			// TODO log trace
		}
		else {
			utils::log::warn("adapter.save_map.forgotten_behaviour_type", "file"_a = path.generic_string());
			continue;
		}
	}

	// Activator
	for (unsigned int i = 0; i < world.interactions.size(); ++i) {
		unsigned int pos_x = std::numeric_limits<unsigned int>::max();
		unsigned int pos_y = std::numeric_limits<unsigned int>::max();

		// scanning for interaction coordinates
		for (unsigned int x_i = 0; x_i < world.map.width() && pos_y == std::numeric_limits<unsigned int>::max(); ++x_i) {
			for (unsigned int y_i = 0; y_i < world.map.height(); ++y_i) {
				if (auto ihandle = world.map[x_i][y_i].interaction_handle; ihandle && *ihandle == i) {
					pos_x = x_i;
					pos_y = y_i;
					break;
				}
			}
		}
		if (pos_x == std::numeric_limits<unsigned int>::max()) {
			utils::log::error("adapter.save_map.interaction_pos_not_found", "index"_a = i); // FIXME add key to lang files
			continue;
		}

		const auto &interaction = world.interactions[i];
		const auto &activator   = world.activators[interaction.interactable_handler];
		const std::string name  = m_view2name.at(m_model2view.at(model_handle{interaction.interactable_handler, model_handle::ACTIVATOR}));

		// building targets ids
		std::vector<std::string> targets;
		targets.reserve(activator.targets.size());
		for (auto target : activator.targets) {
			targets.push_back(m_view2name.at(m_model2view.at(model_handle{target, model_handle::ACTIONABLE})));
		}


		// Printing
		os << "\t[[actors.spawn]]\n"
		   << "\t\t" << map_file::keys::name << " = \"" << name << "\"\n"
		   << "\t\t" << map_file::keys::type << " = \"";
		switch (interaction.interactable) {
			case model::interactable_kind::BUTTON:
				os << map_file::values::button;
				break;
			case model::interactable_kind::INDUCTION_LOOP:
				os << map_file::values::induction_loop;
				break;
			case model::interactable_kind::INFRARED_LASER:
				os << map_file::values::infrared_laser;
				break;
			default:
				os << map_file::values::unknown;
				utils::log::warn("adapter.save_map.forgotten_interactable_type"); // FIXME add key to lang files
				break;
		}
		os << "\"\n"
		   << "\t\t" << map_file::keys::x_pos << " = " << pos_x << "\n"
		   << "\t\t" << map_file::keys::y_pos << " = " << pos_y << "\n"
		   << "\t\t" << map_file::keys::activation_difficulty << " = " << activator.activation_difficulty << "\n"
		   << "\t\t" << map_file::keys::delay << " = " << activator.activation_delay << "\n"
		   << "\t\t" << map_file::keys::refire_repeat << " = " << activator.refire_repeat << "\n";

		if (activator.refire_after) {
			os << "\t\t" << map_file::keys::refire_after << " = " << *activator.refire_after << "\n";
		}

		if (!targets.empty()) {
			os << "\t\t" << map_file::keys::acts_on_table << " = [";
			for (unsigned int j = 0 ; j + 1 < targets.size() ; ++j) {
				os << "\"" << targets[j] << "\", ";
			}
			os << "\"" << targets.back() << "\"]\n";
		}
	}

	// Target
	os << "[target.location]\n"
	   << "\t" << map_file::keys::x_pos << " = " << world.target_tile.x << "\n"
	   << "\t" << map_file::keys::y_pos << " = " << world.target_tile.y << "\n"
	   << "\n";

	// Map
	auto print_tile = [&os](view::cell tile) {

		switch(tile) { // FIXME base it on view rather than model
			case view::cell::iron_tile:
				os << '~';
				break;
			case view::cell::concrete_tile:
				os << ' ';
				break;
			case view::cell::abyss:
				os << '#';
				break;
			default:
				os << '?';
				utils::log::error("adapter.save_map.forgotten_tile_type", "tile"_a = static_cast<int>(tile)); // FIXME add key to lang files
		}
	};

	os << "[map]\n"
	   << "\t" << map_file::keys::layout << " = [\n";
	auto cells = state::access<adapter>::view(m_state).get_cells();
	for (unsigned int y = 0 ; y + 1 < cells.back().size() ; ++y) {
		os << "\t\t\"";
		for (unsigned int x = 0 ; x < cells.size() ; ++x) {
			assert(cells[x].size() == cells.back().size()); // assuming a rectangular map
			print_tile(cells[x][y]);
		}
		os << "\",\n";
	}
	os << "\t\t\"";
	for (unsigned int x = 0 ; x < world.map.width() ; ++x) {
		print_tile(cells[x].back());
	}
	os << "\"\n\t]\n";
}
