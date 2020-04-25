#ifndef NINJACLOWN_UTILS_RESOURCE_MANAGER_HPP
#define NINJACLOWN_UTILS_RESOURCE_MANAGER_HPP

#include <filesystem>
#include <forward_list>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics/Texture.hpp>

#include <cpptoml.h>

#include "terminal_commands.hpp"
#include "utils/optional.hpp"
#include "view/animation.hpp"
#include "view/mob_animations.hpp"

namespace utils {

class resource_manager {
	static constexpr std::string_view DEFAULT_ASSET_FILE = "resources/assets.png";

	struct tiles_infos_t {
		int xspacing;
		int x_yshift;
		int yspacing;
		int y_xshift;
		int width;
		int height;
	} m_tiles_infos;

public:

	enum class mob_id {
		player = MOBS_PLAYERID,
	};

	enum class object_id {
		button = OBJECTS_BUTTONID,
	};

	enum class tile_id {
		chasm    = TILES_CHASMID,
		iron     = TILES_IRONID,
		concrete = TILES_CONCRETEID,
		frame    = TILES_FRAMEID,
	};

	[[nodiscard]] bool load_config(const std::filesystem::path &path) noexcept;

    [[nodiscard]] bool reload(const std::filesystem::path &path) noexcept {
		resource_manager new_config{};
		if (new_config.load_config(path)) {
			*this = std::move(new_config);
			return true;
		}
		return false;
	}

	[[nodiscard]] utils::optional<const view::animation &> tile_animation(tile_id) const noexcept;

	[[nodiscard]] utils::optional<const view::shifted_animation &> object_animation(object_id) const noexcept;

	[[nodiscard]] utils::optional<const view::mob_animations &> mob_animations(mob_id) const noexcept;

    [[nodiscard]] std::optional<std::pair<std::string_view, std::string_view>> text_for(command_id) const noexcept;

	[[nodiscard]] const tiles_infos_t &tiles_infos() const noexcept {
		return m_tiles_infos;
	}

private:
	[[nodiscard]] bool load_graphics(std::shared_ptr<cpptoml::table> config) noexcept;

	[[nodiscard]] bool load_mobs_anims(const std::shared_ptr<cpptoml::table> &mobs_config) noexcept;
	[[nodiscard]] bool load_mob_anim(const std::shared_ptr<cpptoml::table> &mob_anim_config, std::string_view mob_name,
	                                 view::facing_direction::type dir, view::mob_animations &anims, sf::Texture &) noexcept;

	[[nodiscard]] bool load_tiles_anims(const std::shared_ptr<cpptoml::table> &tiles_config) noexcept;

	[[nodiscard]] bool load_objects_anims(const std::shared_ptr<cpptoml::table> &objects_config) noexcept;

	[[nodiscard]] bool load_texts(const std::shared_ptr<cpptoml::table>& config, const std::filesystem::path& resources_directory) noexcept;
	[[nodiscard]] bool load_command_texts(const std::shared_ptr<cpptoml::table>& lang_file) noexcept;

	sf::Texture* get_texture(const std::string& file) noexcept;

	std::unordered_map<std::string, sf::Texture *> m_textures_by_file{};
	std::forward_list<sf::Texture> m_textures_holder{};

	std::unordered_map<tile_id, view::animation> m_tiles_anims{};
	std::unordered_map<object_id, view::shifted_animation> m_objects_anims{};
	std::unordered_map<mob_id, view::mob_animations> m_mobs_anims{};

	std::unordered_map<command_id, std::pair<std::string, std::string>> m_commands_strings{};
};
} // namespace utils

#endif //NINJACLOWN_UTILS_RESOURCE_MANAGER_HPP
