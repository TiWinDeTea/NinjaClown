#include <cpptoml.h>
#include <spdlog/spdlog.h>

#include "utils/logging.hpp"

#include "utils/resource_manager.hpp"
#include "utils/resources_type.hpp"

using utils::optional;
using utils::resource_manager;

namespace {
std::shared_ptr<cpptoml::table> parse_file(const std::string &path) {
	try {
		return cpptoml::parse_file(path);
	}
	catch (const cpptoml::parse_exception &e) {
		spdlog::error("{}", e.what());
		return {};
	}
}

auto parse_file(const std::filesystem::path &path) {
	return parse_file(path.generic_string());
}

constexpr const char lang_folder[] = "lang/";

namespace error_msgs {
	constexpr const char loading_failed[] = "Error parsing config file";
	constexpr const char missing_table[]  = "table is missing";
	constexpr const char missing_array[]  = "array is missing";
	constexpr const char missing_key[]    = "key is missing";
	constexpr const char bad_image_file[] = "file is missing or corrupted";
} // namespace error_msgs

namespace config_keys {
	constexpr const char graphics[] = "graphics";
	constexpr const char id[]       = "id";
	constexpr const char file[]     = "file";
	constexpr const char list[]     = "list";

	namespace sprites {
		constexpr const char frame_count[] = "frame-count";
		constexpr const char pos_x[]       = "pos.x";
		constexpr const char pos_y[]       = "pos.y";
		constexpr const char width[]       = "width";
		constexpr const char height[]      = "height";
		constexpr const char xshift[]      = "shift.x";
		constexpr const char yshift[]      = "shift.y";
	} // namespace sprites

	namespace mobs {
		constexpr const char anims[] = "mobs";

		constexpr const char dir_north[] = "upwards";
		constexpr const char dir_south[] = "downwards";
		constexpr const char dir_east[]  = "rightwards";
		constexpr const char dir_west[]  = "leftwards";
	} // namespace mobs

	namespace tiles {
		constexpr const char anims[]    = "tiles";
		constexpr const char xspacing[] = "spacing.x";
		constexpr const char yspacing[] = "spacing.y";
		constexpr const char x_yshift[] = "x-yshift";
		constexpr const char y_xshift[] = "y-xshift";
	} // namespace tiles

	namespace objects {
		constexpr const char anims[] = "objects";
	}

	namespace user {
		constexpr const char main_table[]    = "user";
		constexpr const char general_lang[]  = "language.general";
        constexpr const char commands_lang[] = "language.commands";
        constexpr const char gui_lang[] = "language.gui";
		// TODO: .general = IHM & Carte + défauts pour les commandes et log
		//       .commands
		//       .log
	} // namespace user

	namespace lang::internal::commands {
		constexpr const char main_name[]   = "commands";
		constexpr const char name[]        = "name";
		constexpr const char description[] = "desc";
	} // namespace lang::internal::commands

	namespace lang::internal::log {
		constexpr const char main_name[] = "log.entry";
		constexpr const char id[]        = "id";
		constexpr const char text[]      = "fmt";
	} // namespace lang::internal::log

    namespace lang::internal::tooltip {
        constexpr const char main_name[] = "tooltip.entry";
        constexpr const char id[]        = "id";
        constexpr const char text[]      = "fmt";
    } // namespace lang::internal::tooltip

    namespace lang::internal::gui {
        constexpr const char main_name[] = "gui.entry";
        constexpr const char id[]        = "id";
        constexpr const char text[]      = "fmt";
    } // namespace lang::internal::tooltip

	namespace lang::internal::variables {
		constexpr const char main_name[] = "variables";
		constexpr const char name[]      = "name";
	} // namespace lang::internal::variables

} // namespace config_keys

optional<view::animation> load_animation(const std::shared_ptr<cpptoml::table> &anim_config, sf::Texture &texture,
                                         std::string_view anim_type, std::string_view anim_name) {
	optional<view::animation> ans{};

	namespace spr    = config_keys::sprites;
	auto frame_count = anim_config->get_qualified_as<int>(spr::frame_count);
	auto pos_x       = anim_config->get_qualified_as<int>(spr::pos_x);
	auto pos_y       = anim_config->get_qualified_as<int>(spr::pos_y);
	auto width       = anim_config->get_qualified_as<int>(spr::width);
	auto height      = anim_config->get_qualified_as<int>(spr::height);

	auto missing_key = [&](std::string_view key) {
		spdlog::error("{}: {}.{}.{}:{} {}", error_msgs::loading_failed, config_keys::graphics, anim_type, anim_name, key,
		              error_msgs::missing_key);
	};
	if (!frame_count) {
		missing_key(spr::frame_count);
		return ans;
	}
	if (!pos_x) {
		missing_key(spr::pos_x);
		return ans;
	}
	if (!pos_y) {
		missing_key(spr::pos_y);
		return ans;
	}
	if (!width) {
		missing_key(spr::width);
		return ans;
	}
	if (!height) {
		missing_key(spr::height);
		return ans;
	}

	view::animation &animation = ans.emplace();
	for (int i = 0; i < *frame_count; ++i) {
		animation.add_frame({texture, {*pos_x + *width * i, *pos_y, *width, *height}});
	}
	return {animation};
}

} // namespace

bool resource_manager::load_config(const std::filesystem::path &path) noexcept {
	auto config = parse_file(path.generic_u8string());
	if (!config) {
		return false;
	}
	return load_graphics(config) && load_texts(config, path.parent_path());
}

optional<const view::animation &> resource_manager::tile_animation(resources_type::tile_id tile) const noexcept {
	auto it = m_tiles_anims.find(tile);
	if (it == m_tiles_anims.end()) {
		return {};
	}
	return {it->second};
}

optional<const view::shifted_animation &> resource_manager::object_animation(resources_type::object_id object) const noexcept {
	auto it = m_objects_anims.find(object);
	if (it == m_objects_anims.end()) {
		return {};
	}
	return {it->second};
}

optional<const view::mob_animations &> resource_manager::mob_animations(resources_type::mob_id mob) const noexcept {
	auto it = m_mobs_anims.find(mob);
	if (it == m_mobs_anims.end()) {
		return {};
	}
	return {it->second};
}

utils::optional<std::pair<std::string_view, std::string_view>> resource_manager::text_for(command_id cmd) const noexcept {
	auto it = m_commands_strings.find(cmd);
	if (it == m_commands_strings.end()) {
		return {};
	}
	return {it->second};
}

utils::optional<std::string_view> resource_manager::log_for(std::string_view key) const noexcept {
	auto it = m_log_strings.find(key);
	if (it != m_log_strings.end()) {
		return {it->second};
	}
	return {};
}

utils::optional<std::string_view> resource_manager::tooltip_for(std::string_view key) const noexcept {
	auto it = m_tooltip_strings.find(key);
	if (it != m_tooltip_strings.end()) {
		return {it->second};
	}
	return {};
}

utils::optional<std::string_view> resource_manager::gui_text_for(std::string_view key) const noexcept {
	auto it = m_gui_strings.find(key);
	if (it != m_gui_strings.end()) {
		return {it->second};
	}
	return {};
}

bool resource_manager::load_graphics(std::shared_ptr<cpptoml::table> config) noexcept {
	config = config->get_table(config_keys::graphics);
	if (!config) {
		spdlog::critical("{}: \"{}\" {}", error_msgs::loading_failed, config_keys::graphics, error_msgs::missing_table);
		return false;
	}

	auto mobs_config    = config->get_table(config_keys::mobs::anims);
	auto tiles_config   = config->get_table(config_keys::tiles::anims);
	auto objects_config = config->get_table(config_keys::objects::anims);

	auto missing_table = [&](std::string_view table) {
		spdlog::error("{}: \"{}.{}\" {}", error_msgs::loading_failed, config_keys::graphics, table, error_msgs::missing_table);
	};
	if (!mobs_config) {
		missing_table(config_keys::mobs::anims);
		return false;
	}
	if (!tiles_config) {
		missing_table(config_keys::tiles::anims);
		return false;
	}
	if (!objects_config) {
		missing_table(config_keys::objects::anims);
		return false;
	}

	bool success = load_tiles_anims(tiles_config);
	success      = load_mobs_anims(mobs_config) && success;
	success      = load_objects_anims(objects_config) && success;
	return success;
}

bool resource_manager::load_mobs_anims(const std::shared_ptr<cpptoml::table> &mobs_config) noexcept {
	namespace mobs = config_keys::mobs;

	cpptoml::option<std::vector<std::string>> mob_list = mobs_config->get_array_of<std::string>(config_keys::list);
	if (!mob_list) {
		spdlog::critical("{}: \"{}.{}\" {}", error_msgs::loading_failed, config_keys::graphics, mobs::anims, error_msgs::missing_array);
		return false;
	}

	bool success = true;
	for (const std::string &mob : *mob_list) {
		auto current_mob = mobs_config->get_table(mob);
		if (!current_mob) {
			success = false;
			spdlog::critical("{}: \"{}.{}.{}\" {}", error_msgs::loading_failed, config_keys::graphics, mobs::anims, mob,
			                 error_msgs::missing_table);
			continue;
		}

		cpptoml::option id = current_mob->get_qualified_as<unsigned int>(config_keys::id);
		if (!id) {
			spdlog::critical("{}: \"{}.{}.{}:{}\" {}", error_msgs::loading_failed, config_keys::graphics, mobs::anims, mob, config_keys::id,
			                 error_msgs::missing_key);
			success = false;
			continue;
		}

		sf::Texture *texture = get_texture(current_mob->get_qualified_as<std::string>(config_keys::file).value_or(DEFAULT_ASSET_FILE));
		if (texture == nullptr) {
			success = false;
			continue;
		}

		view::mob_animations mob_anims;
		auto try_load = [&](view::facing_direction::type dir, const char *dir_str) {
			auto anim_config = current_mob->get_table(dir_str);
			if (anim_config) {
				success = load_mob_anim(anim_config, mob, dir, mob_anims, *texture) && success;
			}
			else {
				spdlog::error("{}: \"{}.{}.{}.{}\" {}", error_msgs::loading_failed, error_msgs::loading_failed, config_keys::graphics,
				              mobs::anims, mob, dir_str, error_msgs::missing_table);
				success = false;
			}
		};

		try_load(view::facing_direction::N, mobs::dir_north);
		try_load(view::facing_direction::S, mobs::dir_south);
		try_load(view::facing_direction::E, mobs::dir_east);
		try_load(view::facing_direction::W, mobs::dir_west);

		m_mobs_anims.emplace(static_cast<resources_type::mob_id>(*id), std::move(mob_anims));
	}

	return success;
}

bool resource_manager::load_mob_anim(const std::shared_ptr<cpptoml::table> &mob_anim_config, std::string_view mob_name,
                                     view::facing_direction::type dir, view::mob_animations &anims, sf::Texture &texture) noexcept {
	auto anim = load_animation(mob_anim_config, texture, config_keys::mobs::anims, mob_name);
	if (!anim) {
		return false;
	}

	namespace spr = config_keys::sprites;
	view::shifted_animation shifted_anim{std::move(*anim)};
	shifted_anim.set_shift(static_cast<float>(mob_anim_config->get_qualified_as<int>(spr::xshift).value_or(0)),
	                       static_cast<float>(mob_anim_config->get_qualified_as<int>(spr::yshift).value_or(0)));

	anims.add_animation(std::move(shifted_anim), dir);
	return true;
}

bool resource_manager::load_tiles_anims(const std::shared_ptr<cpptoml::table> &tiles_config) noexcept {
	namespace tiles = config_keys::tiles;
	namespace spr   = config_keys::sprites;

	auto tile_list = tiles_config->get_array_of<std::string>(config_keys::list);
	auto xspacing  = tiles_config->get_qualified_as<int>(tiles::xspacing);
	auto x_yshift  = tiles_config->get_qualified_as<int>(tiles::x_yshift);
	auto yspacing  = tiles_config->get_qualified_as<int>(tiles::yspacing);
	auto y_xshift  = tiles_config->get_qualified_as<int>(tiles::y_xshift);
	auto width     = tiles_config->get_qualified_as<int>(spr::width);
	auto height    = tiles_config->get_qualified_as<int>(spr::height);

	auto missing_key = [](std::string_view key) {
		spdlog::error("{}: \"{}.{}.{}\" {}", error_msgs::loading_failed, config_keys::graphics, tiles::anims, key, error_msgs::missing_key);
	};
	if (!tile_list) {
		missing_key(config_keys::list);
		return false;
	}
	if (!xspacing) {
		missing_key(tiles::xspacing);
		return false;
	}
	if (!x_yshift) {
		missing_key(tiles::x_yshift);
		return false;
	}
	if (!yspacing) {
		missing_key(tiles::yspacing);
		return false;
	}
	if (!y_xshift) {
		missing_key(tiles::y_xshift);
		return false;
	}
	if (!width) {
		missing_key(spr::width);
		return false;
	}
	if (!height) {
		missing_key(spr::height);
		return false;
	}
	m_tiles_infos.xspacing = *xspacing;
	m_tiles_infos.x_yshift = *x_yshift;
	m_tiles_infos.yspacing = *yspacing;
	m_tiles_infos.y_xshift = *y_xshift;
	m_tiles_infos.width    = *width;
	m_tiles_infos.height   = *height;

	bool success = true;
	for (const std::string &tile : *tile_list) {
		auto missing_tile_key = [&tile](std::string_view key) {
			spdlog::error("{}: \"{}.{}.{}.{}\" {}", error_msgs::loading_failed, config_keys::graphics, tiles::anims, tile, key,
			              error_msgs::missing_key);
		};

		auto current_tile = tiles_config->get_table(tile);
		if (!current_tile) {
			spdlog::error("{}: \"{}.{}.{}\" {}", error_msgs::loading_failed, config_keys::graphics, tiles::anims, tile,
			              error_msgs::missing_table);
			success = false;
			continue;
		}

		auto id          = current_tile->get_qualified_as<int>(config_keys::id);
		auto frame_count = current_tile->get_qualified_as<int>(spr::frame_count);
		auto pos_x       = current_tile->get_qualified_as<int>(spr::pos_x);
		auto pos_y       = current_tile->get_qualified_as<int>(spr::pos_y);
		if (!id) {
			missing_tile_key(config_keys::id);
			success = false;
			continue;
		}
		if (!frame_count) {
			missing_tile_key(spr::frame_count);
			success = false;
			continue;
		}
		if (!pos_x) {
			missing_tile_key(spr::pos_x);
			success = false;
			continue;
		}
		if (!pos_y) {
			missing_tile_key(spr::pos_y);
			success = false;
			continue;
		}

		sf::Texture *texture = get_texture(current_tile->get_qualified_as<std::string>(config_keys::file).value_or(DEFAULT_ASSET_FILE));
		if (texture == nullptr) {
			success = false;
			continue;
		}

		view::animation animation;
		for (int i = 0; i < *frame_count; ++i) {
			animation.add_frame({*texture, {*pos_x + *width * i, *pos_y, *width, *height}});
		}
		m_tiles_anims.emplace(static_cast<resources_type::tile_id>(*id), std::move(animation));
	}
	return success;
}

bool resource_manager::load_objects_anims(const std::shared_ptr<cpptoml::table> &objects_config) noexcept {
	namespace objects = config_keys::objects;
	namespace spr     = config_keys::sprites;

	auto object_list = objects_config->get_array_of<std::string>(config_keys::list);
	if (!object_list) {
		spdlog::error("{}: \"{}.{}.{}\" {}", error_msgs::loading_failed, config_keys::graphics, objects::anims, config_keys::list,
		              error_msgs::missing_key);
		return false;
	}

	bool success = true;
	for (const std::string &object : *object_list) {
		auto missing_key = [&object](std::string_view key) {
			spdlog::error("{}: \"{}.{}.{}.{}\" {}", error_msgs::loading_failed, config_keys::graphics, objects::anims, object, key,
			              error_msgs::missing_key);
		};

		auto current_object = objects_config->get_table(object);
		if (!current_object) {
			spdlog::error("{}: \"{}.{}.{}\" {}", error_msgs::loading_failed, config_keys::graphics, objects::anims, object,
			              error_msgs::missing_table);
			success = false;
			continue;
		}

		auto id          = current_object->get_qualified_as<int>(config_keys::id);
		auto frame_count = current_object->get_qualified_as<int>(spr::frame_count);
		auto pos_x       = current_object->get_qualified_as<int>(spr::pos_x);
		auto pos_y       = current_object->get_qualified_as<int>(spr::pos_y);
		auto width       = current_object->get_qualified_as<int>(spr::width);
		auto height      = current_object->get_qualified_as<int>(spr::height);
		if (!id) {
			missing_key(config_keys::id);
			success = false;
			continue;
		}
		if (!frame_count) {
			missing_key(spr::frame_count);
			success = false;
			continue;
		}
		if (!pos_x) {
			missing_key(spr::pos_x);
			success = false;
			continue;
		}
		if (!pos_y) {
			missing_key(spr::pos_y);
			success = false;
			continue;
		}
		if (!width) {
			missing_key(spr::width);
			success = false;
			continue;
		}
		if (!height) {
			missing_key(spr::height);
			success = false;
			continue;
		}

		sf::Texture *texture = get_texture(current_object->get_qualified_as<std::string>(config_keys::file).value_or(DEFAULT_ASSET_FILE));
		if (texture == nullptr) {
			success = false;
			continue;
		}

		auto xshift = current_object->get_qualified_as<int>(spr::xshift);
		auto yshift = current_object->get_qualified_as<int>(spr::yshift);

		view::shifted_animation animation;
		for (int i = 0; i < *frame_count; ++i) {
			animation.add_frame({*texture, {*pos_x + *width * i, *pos_y, *width, *height}});
		}
		animation.set_shift(static_cast<float>(xshift.value_or(0)), static_cast<float>(yshift.value_or(0)));
		m_objects_anims.emplace(static_cast<resources_type::object_id>(*id), std::move(animation));
	}
	return success;
}

bool resource_manager::load_texts(const std::shared_ptr<cpptoml::table> &config,
                                  const std::filesystem::path &resources_directory) noexcept {
	namespace user = config_keys::user;

	auto lang_config = config->get_table(config_keys::user::main_table);
	if (!lang_config) {
		spdlog::error("{}: \"{}\" {}", error_msgs::loading_failed, user::main_table, error_msgs::missing_table);
		return false;
	}

	auto general_lang  = lang_config->get_qualified_as<std::string>(user::general_lang);
    auto commands_lang = lang_config->get_qualified_as<std::string>(user::commands_lang);
    auto gui_lang = lang_config->get_qualified_as<std::string>(user::gui_lang);

	if (!general_lang) {
		spdlog::error(R"({}: "{}.{}" {})", error_msgs::loading_failed, user::main_table, user::general_lang, error_msgs::missing_key);
		return false;
	}

	if (!commands_lang) {
		commands_lang = general_lang;
	}

	std::filesystem::path path                    = resources_directory / lang_folder / (*general_lang + ".toml");
	std::shared_ptr<cpptoml::table> log_text_file = parse_file(path.generic_string());
	if (!log_text_file) {
		return false;
	}

    path                                               = resources_directory / lang_folder / (*commands_lang + ".toml");
    std::shared_ptr<cpptoml::table> commands_text_file = parse_file(path.generic_string());
    if (!commands_text_file) {
        return false;
    }

    path                                               = resources_directory / lang_folder / (*gui_lang + ".toml");
    std::shared_ptr<cpptoml::table> gui_text_file = parse_file(path.generic_string());
    if (!gui_text_file) {
        return false;
    }


    bool success = load_logging_texts(log_text_file);
	success      = load_tooltip_texts(log_text_file) && success;
	success      = load_command_texts(commands_text_file) && success;
	success      = load_gui_texts(gui_text_file) && success;
	return success;
}

bool resource_manager::load_command_texts(const std::shared_ptr<cpptoml::table> &lang_file) noexcept {
	namespace cmds = config_keys::lang::internal::commands;

	bool result = true;
	for (int i = 0; i < static_cast<int>(command_id::OUTOFRANGE); ++i) {
		auto cmd = lang_file->get_table_qualified(cmds::main_name + ("." + std::to_string(i)));
		if (!cmd) {
			spdlog::error(R"({}: cmd lang file: "{}.{}" {})", error_msgs::loading_failed, cmds::main_name, i, error_msgs::missing_key);
			result = false;
			continue;
		}

		auto name = cmd->get_qualified_as<std::string>(cmds::name);
		auto desc = cmd->get_qualified_as<std::string>(cmds::description);

		if (!name) {
			spdlog::error(R"({}: cmd lang file: "{}.{}.{}" {})", error_msgs::loading_failed, cmds::main_name, i, cmds::name,
			              error_msgs::missing_key);
			result = false;
		}
		if (!desc) {
			spdlog::error(R"({}: cmd lang file: "{}.{}.{}" {})", error_msgs::loading_failed, cmds::main_name, i, cmds::description,
			              error_msgs::missing_key);
			result = false;
		}

		m_commands_strings.emplace(static_cast<command_id>(i), std::pair{*name, *desc});
	}
	return result;
}

bool resource_manager::load_logging_texts(const std::shared_ptr<cpptoml::table> &lang_file) noexcept {
	namespace log_ns = config_keys::lang::internal::log;

	std::shared_ptr<cpptoml::table_array> logs = lang_file->get_table_array_qualified(log_ns::main_name);
	return generic_load_keyed_texts(logs, log_ns::id, log_ns::text, m_log_strings, m_log_string_keys);
}

bool resource_manager::load_tooltip_texts(const std::shared_ptr<cpptoml::table> &lang_file) noexcept {
	namespace tooltip_ns = config_keys::lang::internal::tooltip;

	std::shared_ptr<cpptoml::table_array> tooltips = lang_file->get_table_array_qualified(tooltip_ns::main_name);
	return generic_load_keyed_texts(tooltips, tooltip_ns::id, tooltip_ns::text, m_tooltip_strings, m_tooltip_string_keys);
}


bool resource_manager::load_gui_texts(const std::shared_ptr<cpptoml::table> &lang_file) noexcept {
    namespace gui_ns = config_keys::lang::internal::gui;

    std::shared_ptr<cpptoml::table_array> gui_strings = lang_file->get_table_array_qualified(gui_ns::main_name);
    return generic_load_keyed_texts(gui_strings, gui_ns::id, gui_ns::text, m_gui_strings, m_gui_string_keys);
}

sf::Texture *resource_manager::get_texture(const std::string &file) noexcept {
	auto it = m_textures_by_file.find(file);
	if (it != m_textures_by_file.end()) {
		return it->second;
	}

	m_textures_holder.emplace_front();
	sf::Texture *texture = &m_textures_holder.front();
	if (!texture->loadFromFile(file)) {
		m_textures_holder.pop_front();
		spdlog::error("{}: \"{}\": {}", error_msgs::loading_failed, file, error_msgs::bad_image_file);
		return nullptr;
	}

	m_textures_by_file.emplace(file, texture);
	return texture;
}

bool resource_manager::generic_load_keyed_texts(const std::shared_ptr<cpptoml::table_array> &table_array, const char *id_key,
                                                const char *text_key, std::unordered_map<std::string_view, std::string> &strings_out,
                                                std::vector<std::string> &keys_out) noexcept {
	if (table_array == nullptr) {
		return false;
	}

	assert(keys_out.empty());
	assert(strings_out.empty());

	bool result{true};
	keys_out.reserve(table_array->get().size());
	for (std::shared_ptr<cpptoml::table> &value : *table_array) {
		bool just_failed{false};

		cpptoml::option<std::string> id = value->get_qualified_as<std::string>(id_key);
		if (!id) {
			spdlog::warn("{}: {}: {}", error_msgs::loading_failed, error_msgs::missing_key, id_key);
			just_failed = true;
		}

		cpptoml::option<std::string> text = value->get_as<std::string>(text_key);
		if (!text) {
			spdlog::warn("{}: {}: {}", error_msgs::loading_failed, error_msgs::missing_key, text_key);
			just_failed = true;
		}

		if (just_failed) {
			result = false;
		}
		else {
			assert(keys_out.capacity() >= strings_out.size() && "translation strings uses pointers to the strings stored in keys_out");
			keys_out.emplace_back(*id); // please std::move, cpptoml
			strings_out.emplace(keys_out.back(), *text);
		}
	}

	return result;
}
