#ifndef NINJACLOWN_UTILS_RESOURCE_MANAGER_HPP
#define NINJACLOWN_UTILS_RESOURCE_MANAGER_HPP

#include <filesystem>
#include <forward_list>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics/Texture.hpp>
#include <cpptoml/cpptoml.h>

#include "terminal_ids.hpp"
#include "utils/optional.hpp"
#include "view/facing_dir.hpp"

#include "view/animation.hpp"
#include "view/mob_animations.hpp"

namespace utils {

namespace resources_type {
	enum class mob_id;

	enum class object_id;

	enum class tile_id;
} // namespace resources_type

// TODO Effets sonores
// TODO Choix de police d’écriture (principalement pour le support des caractères)
// TODO éclater en plusieurs classes ?

class resource_manager {
	static constexpr std::string_view DEFAULT_ASSET_FILE = "assets.png";
	static constexpr std::string_view CONFIG_FILE = "config.toml";

	struct tiles_infos_t {
		int xspacing;
		int x_yshift;
		int yspacing;
		int y_xshift;
		int width;
		int height;
	} m_tiles_infos{};

	struct lang_info {
		std::string name;
		std::string variant;
		std::string shorthand;
		std::string map_name;
		std::filesystem::path file;

		bool operator==(const lang_info& other) const noexcept {
			return file == other.file;
		}
	};

public:

	struct resource_pack_info {
		std::string default_name;
		std::unordered_map<std::string, std::string> names_by_shorthand_lang;
		std::filesystem::path file;
	};

	[[nodiscard]] bool load_config() noexcept;

	[[nodiscard]] bool reload() noexcept {
		resource_manager new_config{};
		if (new_config.load_config()) {
			*this = std::move(new_config);
			return true;
		}
		return false;
	}

	[[nodiscard]] utils::optional<const view::animation &> tile_animation(resources_type::tile_id) const noexcept;

	[[nodiscard]] utils::optional<const view::shifted_animation &> object_animation(resources_type::object_id) const noexcept;

	[[nodiscard]] utils::optional<const view::mob_animations &> mob_animations(resources_type::mob_id) const noexcept;

	[[nodiscard]] utils::optional<std::pair<std::string_view, std::string_view>> text_for(command_id) const noexcept;

	[[nodiscard]] std::string_view log_for(std::string_view key) const noexcept;

	[[nodiscard]] std::string_view tooltip_for(std::string_view key) const noexcept;

	[[nodiscard]] std::string_view gui_text_for(std::string_view key) const noexcept;

	[[nodiscard]] const tiles_infos_t &tiles_infos() const noexcept {
		return m_tiles_infos;
	}

	/**
	 * @return a cached list of known language (according to language files within the lang folder)
	 * @see refresh_language_list
	 */
	[[nodiscard]] const std::vector<lang_info>& get_language_list() const noexcept {
		return m_available_langs;
	}

	/**
	 * @return a cached list of known resource packs (according to resource pack files within the resource pack folder)
	 * @see refresh_resource_pack_list
	 */
    [[nodiscard]] const std::vector<resource_pack_info>& get_resource_pack_list() const noexcept {
		return m_resource_packs;
    }

	/**
	 * Refreshes the known language cache
	 * @see get_language_list
	 */
	void refresh_language_list();
	void refresh_resource_pack_list();

	[[nodiscard]] const lang_info& user_general_lang() const noexcept {
		return m_user_general_lang;
	}
	[[nodiscard]] const lang_info& user_commands_lang() const noexcept {
		return m_user_command_lang;
	}
	[[nodiscard]] const lang_info& user_gui_lang() const noexcept {
		return m_user_gui_lang;
	}
	[[nodiscard]] const lang_info& user_log_lang() const noexcept {
		return m_user_log_lang;
	}

	[[nodiscard]] const resource_pack_info& user_resource_pack() const noexcept {
	    return m_user_resource_pack;
  }

	void set_user_general_lang(const lang_info&) noexcept;
	void set_user_command_lang(const lang_info&) noexcept;
	void set_user_gui_lang(const lang_info&) noexcept;
	void set_user_log_lang(const lang_info&) noexcept;
	void set_user_resource_pack(const resource_pack_info&) noexcept;

	bool save_user_config() const noexcept;

private:
	[[nodiscard]] bool load_graphics(std::shared_ptr<cpptoml::table> config, const std::filesystem::path &resourcepack_directory) noexcept;

	[[nodiscard]] bool load_tiles_anims(const std::shared_ptr<cpptoml::table> &tiles_config, const std::string &graph_file) noexcept;
	[[nodiscard]] bool load_objects_anims(const std::shared_ptr<cpptoml::table> &objects_config, const std::string &graph_file) noexcept;
	[[nodiscard]] bool load_mobs_anims(const std::shared_ptr<cpptoml::table> &mobs_config, const std::string &graph_file) noexcept;
	[[nodiscard]] bool load_mob_anim(const std::shared_ptr<cpptoml::table> &mob_anim_config, std::string_view mob_name,
	                                 view::facing_direction::type dir, view::mob_animations &anims, sf::Texture &) noexcept;

	[[nodiscard]] bool load_texts(const std::shared_ptr<cpptoml::table> &config) noexcept;
	[[nodiscard]] bool load_command_texts(const std::shared_ptr<cpptoml::table> &lang_file) noexcept;
	[[nodiscard]] bool load_logging_texts(const std::shared_ptr<cpptoml::table> &lang_file) noexcept;
	[[nodiscard]] bool load_tooltip_texts(const std::shared_ptr<cpptoml::table> &lang_file) noexcept;
	[[nodiscard]] bool load_gui_texts(const std::shared_ptr<cpptoml::table> &lang_file) noexcept;

	[[nodiscard]] static bool generic_load_keyed_texts(const std::shared_ptr<cpptoml::table_array> &table_array, const char *id_key,
	                                                   const char *text_key, std::unordered_map<std::string_view, std::string> &strings_out,
	                                                   std::vector<std::string> &keys_out) noexcept;

	sf::Texture *get_texture(const std::string &file) noexcept;

	std::unordered_map<std::string, sf::Texture *> m_textures_by_file{};
	std::forward_list<sf::Texture> m_textures_holder{};

	std::unordered_map<resources_type::tile_id, view::animation> m_tiles_anims{};
	std::unordered_map<resources_type::object_id, view::shifted_animation> m_objects_anims{};
	std::unordered_map<resources_type::mob_id, view::mob_animations> m_mobs_anims{};

	std::unordered_map<command_id, std::pair<std::string, std::string>> m_commands_strings{};

	std::unordered_map<std::string_view, std::string> m_log_strings{};
	std::vector<std::string> m_log_string_keys{};
	std::unordered_map<std::string_view, std::string> m_tooltip_strings{};
	std::vector<std::string> m_tooltip_string_keys{};
	std::unordered_map<std::string_view, std::string> m_gui_strings{};
	std::vector<std::string> m_gui_string_keys{};

	std::vector<lang_info> m_available_langs;
	std::vector<resource_pack_info> m_resource_packs;

	// user config (saved by "save_user_config"
	lang_info m_user_general_lang{};
	lang_info m_user_command_lang{};
	lang_info m_user_gui_lang{};
	lang_info m_user_log_lang{};
	resource_pack_info m_user_resource_pack{};
};
} // namespace utils

#endif //NINJACLOWN_UTILS_RESOURCE_MANAGER_HPP
