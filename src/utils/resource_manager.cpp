#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-avoid-c-arrays"
#pragma ide diagnostic ignored "cppcoreguidelines-pro-bounds-array-to-pointer-decay"
#include <cpptoml/cpptoml.h>
#include <spdlog/spdlog.h>

#include <iterator>

#include "utils/logging.hpp"

#include "utils/resource_manager.hpp"
#include "utils/resources_type.hpp"
#include "utils/system.hpp"

using utils::optional;
using utils::resource_manager;

namespace {
std::shared_ptr<cpptoml::table> parse_file(const std::string &path) {
	try {
		return cpptoml::parse_file(path);
	}
	catch (const cpptoml::parse_exception &e) {
		spdlog::error("{} (file: {})", e.what(), path);
		return {};
	}
}

auto parse_file(const std::filesystem::path &path) {
	return parse_file(path.generic_string());
}

constexpr const char lang_folder[]             = "lang";
constexpr const char lang_file_ext[]           = ".toml";
constexpr const char resource_pack_folder[]    = "resource packs";
constexpr const char resource_pack_toml_name[] = "resource_pack.toml";

namespace error_msgs {
	constexpr const char loading_failed[] = "Error parsing config file";
	constexpr const char missing_table[]  = "table is missing";
	constexpr const char missing_array[]  = "array is missing";
	constexpr const char missing_key[]    = "key is missing";
	constexpr const char bad_image_file[] = "file is missing or corrupted";
} // namespace error_msgs

namespace config_keys {
	constexpr const char graphics_resource_file[]     = "user.resource_pack";
	constexpr const char unqualified_graph_res_file[] = "resource_pack";
	constexpr const char graphics[]                   = "graphics";
	constexpr const char graphics_main_file[]         = "file";
	constexpr const char id[]                         = "id";
	constexpr const char file[]                       = "file";
	constexpr const char list[]                       = "list";

	namespace meta {
		constexpr const char name_qualified[]       = "meta.name";
		constexpr const char name_prefix[]          = "name";
		constexpr const char language[]             = "meta.language";
		constexpr const char variant[]              = "meta.variant";
		constexpr const char shorthand[]            = "meta.shorthand";
		constexpr const char maps_to[]              = "meta.mapped_name";
		constexpr const char respack_default_lang[] = "dflt_lang";
	} // namespace meta

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

		constexpr const char dir_north_west[] = "upleftwards";
		constexpr const char dir_south_west[] = "downleftwards";
		constexpr const char dir_north_east[] = "uprightwards";
		constexpr const char dir_south_east[] = "downrightwards";
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
		constexpr const char lang_table[]    = "language";
		constexpr const char general_lang[]  = "language.general";
		constexpr const char commands_lang[] = "language.commands";
		constexpr const char gui_lang[]      = "language.gui";
		constexpr const char log_lang[]      = "language.log";
		namespace non_qualified {
			constexpr const char general_lang[]  = "general";
			constexpr const char commands_lang[] = "commands";
			constexpr const char gui_lang[]      = "gui";
			constexpr const char log_lang[]      = "log";
		} // namespace non_qualified
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
	} // namespace lang::internal::gui

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

template <typename T>
T parse_lang_info_impl(std::filesystem::path &&path) {
	namespace meta = config_keys::meta;

	T ans{};
	try {
		std::shared_ptr<cpptoml::table> table = cpptoml::parse_file(path.generic_string());
		ans.file                              = std::move(path);
		if (!table) {
			return ans;
		}

		cpptoml::option<std::string> lang   = table->get_qualified_as<std::string>(meta::language);
		cpptoml::option<std::string> var    = table->get_qualified_as<std::string>(meta::variant);
		cpptoml::option<std::string> shhand = table->get_qualified_as<std::string>(meta::shorthand);
		cpptoml::option<std::string> map_n  = table->get_qualified_as<std::string>(meta::maps_to);

		if (lang || shhand) {

			if (shhand) {
				ans.shorthand = *shhand;
			}
			if (var) {
				ans.variant = *var;
			}
			if (lang) {
				ans.name = *lang;
			}
			else {
				std::swap(ans.name, ans.shorthand);
			}

			if (map_n) {
				ans.map_name = *map_n;
			}
		}
	}
	catch (const std::exception &e) {
		spdlog::error("{}: {}", path.generic_string(), e.what());
	}
	return ans;
}

template <typename T>
T parse_lang_info(const std::filesystem::path &path) {
	return parse_lang_info_impl<T>(std::move(std::filesystem::path{path}));
}

template <typename T>
T parse_lang_info(std::filesystem::path &&path) {
	return parse_lang_info_impl<T>(std::move(path));
}

resource_manager::resource_pack_info parse_resource_pack_info(std::filesystem::path &&path) {
	resource_manager::resource_pack_info info;
	try {
		std::shared_ptr<cpptoml::table> table = cpptoml::parse_file(path.generic_string());
		info.file                             = std::move(path);
		if (!table) {
			return info;
		}

		namespace meta = config_keys::meta;
		table          = table->get_table_qualified(meta::name_qualified);
		if (!table) {
			return info;
		}

		for (const auto &entry : *table) {
			if (entry.first != meta::respack_default_lang) {
				std::shared_ptr<cpptoml::value<std::string>> name_toml = entry.second->as<std::string>();
				std::string lang                                       = entry.first;

				if (name_toml) {
					info.names_by_shorthand_lang.insert(std::make_pair(std::move(lang), name_toml->get()));
				}
			}
		}

		cpptoml::option<std::string> default_lang = table->get_qualified_as<std::string>(meta::respack_default_lang);
		if (default_lang) {
			if (auto it = info.names_by_shorthand_lang.find(*default_lang); it != info.names_by_shorthand_lang.end()) {
				info.default_name = it->second;
			}
			else {
				info.default_name = "??????";
			}
		}
	}
	catch (const std::exception &e) {
		spdlog::error("{}: {}", path.generic_string(), e.what());
	}
	return info;
}

resource_manager::resource_pack_info parse_resource_pack_info(const std::filesystem::path &path) {
	return parse_resource_pack_info(std::move(std::filesystem::path{path}));
}

} // namespace

bool resource_manager::load_config() noexcept {
	auto config = parse_file(utils::config_directory() / CONFIG_FILE);
	if (!config) {
		return false;
	}

	auto resource_pack = config->get_qualified_as<std::string>(config_keys::graphics_resource_file);
	if (!resource_pack) {
		spdlog::error("No resource pack specified in config file");
		return false;
	}

	auto graphics = parse_file(*resource_pack);
	if (!graphics) {
		return false;
	}
	m_user_resource_pack = parse_resource_pack_info(*resource_pack);
	return load_graphics(graphics, std::filesystem::path{*resource_pack}.parent_path()) && load_texts(config);
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

std::string_view resource_manager::log_for(std::string_view key) const noexcept {
	auto it = m_log_strings.find(key);
	if (it != m_log_strings.end()) {
		return it->second;
	}
	return key;
}

std::string_view resource_manager::tooltip_for(std::string_view key) const noexcept {
	auto it = m_tooltip_strings.find(key);
	if (it != m_tooltip_strings.end()) {
		return it->second;
	}
	return key;
}

std::string_view resource_manager::gui_text_for(std::string_view key) const noexcept {
	auto it = m_gui_strings.find(key);
	if (it != m_gui_strings.end()) {
		return it->second;
	}
	return key;
}

bool resource_manager::load_graphics(std::shared_ptr<cpptoml::table> config, const std::filesystem::path &resourcepack_directory) noexcept {
	assert(m_textures_by_file.empty());
	assert(m_textures_holder.empty());
	assert(m_tiles_anims.empty());
	assert(m_objects_anims.empty());
	assert(m_mobs_anims.empty());

	config = config->get_table(config_keys::graphics);
	if (!config) {
		spdlog::critical("{}: \"{}\" {}", error_msgs::loading_failed, config_keys::graphics, error_msgs::missing_table);
		return false;
	}

	std::string graphics_file;
	if (auto f = config->get_as<std::string>(config_keys::graphics_main_file); f) {
		graphics_file = (resourcepack_directory / *f).generic_string();
	}
	else {
		spdlog::error("No assets file specified");
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

	bool success = load_tiles_anims(tiles_config, graphics_file);
	success      = load_mobs_anims(mobs_config, graphics_file) && success;
	success      = load_objects_anims(objects_config, graphics_file) && success;
	return success;
}

bool resource_manager::load_mobs_anims(const std::shared_ptr<cpptoml::table> &mobs_config, const std::string &graph_file) noexcept {
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

		sf::Texture *texture = get_texture(current_mob->get_qualified_as<std::string>(config_keys::file).value_or(graph_file));
		if (texture == nullptr) {
			success = false;
			continue;
		}

		view::mob_animations mob_anims;
		auto try_load = [&](view::facing_direction::type dir, const char *dir_str, const char *or_else_dir_str = nullptr) {
			auto anim_config = current_mob->get_table(dir_str);
			if (anim_config) {
				success = load_mob_anim(anim_config, mob, dir, mob_anims, *texture) && success;
			}
			else if (or_else_dir_str != nullptr) {
				anim_config = current_mob->get_table(or_else_dir_str);
				if (anim_config) {
					success = load_mob_anim(anim_config, mob, dir, mob_anims, *texture) && success;
				}
				else {
					spdlog::error("{}: \"{}.{}.{}.{}\" {}", error_msgs::loading_failed, error_msgs::loading_failed, config_keys::graphics,
					              mobs::anims, mob, dir_str, error_msgs::missing_table);
					success = false;
				}
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
		try_load(view::facing_direction::NW, mobs::dir_north_west, mobs::dir_north);
		try_load(view::facing_direction::SW, mobs::dir_south_west, mobs::dir_west);
		try_load(view::facing_direction::NE, mobs::dir_north_east, mobs::dir_east);
		try_load(view::facing_direction::SE, mobs::dir_south_east, mobs::dir_south);

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

bool resource_manager::load_tiles_anims(const std::shared_ptr<cpptoml::table> &tiles_config, const std::string &graph_file) noexcept {
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

		sf::Texture *texture = get_texture(current_tile->get_qualified_as<std::string>(config_keys::file).value_or(graph_file));
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

bool resource_manager::load_objects_anims(const std::shared_ptr<cpptoml::table> &objects_config, const std::string &graph_file) noexcept {
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

		sf::Texture *texture = get_texture(current_object->get_qualified_as<std::string>(config_keys::file).value_or(graph_file));
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

bool resource_manager::load_texts(const std::shared_ptr<cpptoml::table> &config) noexcept {
	namespace user = config_keys::user;

	auto lang_config = config->get_table(config_keys::user::main_table);
	if (!lang_config) {
		spdlog::error("{}: \"{}\" {}", error_msgs::loading_failed, user::main_table, error_msgs::missing_table);
		return false;
	}

	auto general_lang  = lang_config->get_qualified_as<std::string>(user::general_lang);
	auto commands_lang = lang_config->get_qualified_as<std::string>(user::commands_lang);
	auto gui_lang      = lang_config->get_qualified_as<std::string>(user::gui_lang);
	auto log_lang      = lang_config->get_qualified_as<std::string>(user::log_lang);

	if (!general_lang) {
		spdlog::error(R"({}: "{}.{}" {})", error_msgs::loading_failed, user::main_table, user::general_lang, error_msgs::missing_key);
		return false;
	}

	std::filesystem::path res_dir = utils::resources_directory();
	{
		std::filesystem::path gen_lang_path(*general_lang);
		if (gen_lang_path.is_relative()) {
			m_user_general_lang = parse_lang_info<lang_info>(res_dir / lang_folder / gen_lang_path);
		}
		else {
			m_user_general_lang = parse_lang_info<lang_info>(gen_lang_path);
		}
	}

	auto parse = [&general_lang, &res_dir](cpptoml::option<std::string> &to_parse, lang_info &related_info) {
		if (!to_parse) {
			to_parse = general_lang;
		}
		std::filesystem::path as_path{*to_parse};
		if (as_path.is_relative()) {
			related_info.file = res_dir / lang_folder / as_path;
		}
		else {
			related_info.file = std::move(as_path);
		}
		return parse_file(related_info.file);
	};

	std::shared_ptr<cpptoml::table> log_text_file = parse(log_lang, m_user_log_lang);
	if (!log_text_file) {
		return false;
	}

	std::shared_ptr<cpptoml::table> commands_text_file = parse(commands_lang, m_user_command_lang);
	if (!commands_text_file) {
		return false;
	}

	std::shared_ptr<cpptoml::table> gui_text_file = parse(gui_lang, m_user_gui_lang);
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

	auto try_read = [&lang_file](const char *key) -> std::string {
		if (auto val = lang_file->get_qualified_as<std::string>(key)) {
			return *val;
		}
		return "?????";
	};

	m_user_command_lang.name      = try_read(config_keys::meta::language);
	m_user_command_lang.variant   = try_read(config_keys::meta::variant);
	m_user_command_lang.shorthand = try_read(config_keys::meta::shorthand);
	m_user_command_lang.map_name  = try_read(config_keys::meta::maps_to);

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

	auto try_read = [&lang_file](const char *key) -> std::string {
		if (auto val = lang_file->get_qualified_as<std::string>(key)) {
			return *val;
		}
		return "?????";
	};

	m_user_log_lang.name      = try_read(config_keys::meta::language);
	m_user_log_lang.variant   = try_read(config_keys::meta::variant);
	m_user_log_lang.shorthand = try_read(config_keys::meta::shorthand);
	m_user_log_lang.map_name  = try_read(config_keys::meta::maps_to);

	std::shared_ptr<cpptoml::table_array> logs = lang_file->get_table_array_qualified(log_ns::main_name);
	return generic_load_keyed_texts(logs, log_ns::id, log_ns::text, m_log_strings, m_log_string_keys);
}

bool resource_manager::load_tooltip_texts(const std::shared_ptr<cpptoml::table> &lang_file) noexcept {
	namespace tooltip_ns = config_keys::lang::internal::tooltip;

	std::shared_ptr<cpptoml::table_array> tooltips = lang_file->get_table_array_qualified(tooltip_ns::main_name);
	return generic_load_keyed_texts(tooltips, tooltip_ns::id, tooltip_ns::text, m_tooltip_strings, m_tooltip_string_keys);
}

bool resource_manager::load_gui_texts(const std::shared_ptr<cpptoml::table> &gui_file) noexcept {
	namespace gui_ns = config_keys::lang::internal::gui;

	auto try_read = [&gui_file](const char *key) -> std::string {
		if (auto val = gui_file->get_qualified_as<std::string>(key)) {
			return *val;
		}
		return "PASTEQ?????";
	};

	m_user_gui_lang.name      = try_read(config_keys::meta::language);
	m_user_gui_lang.variant   = try_read(config_keys::meta::variant);
	m_user_gui_lang.shorthand = try_read(config_keys::meta::shorthand);
	m_user_gui_lang.map_name  = try_read(config_keys::meta::maps_to);

	std::shared_ptr<cpptoml::table_array> gui_strings = gui_file->get_table_array_qualified(gui_ns::main_name);
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

void resource_manager::refresh_language_list() {
	std::vector<std::filesystem::path> language_directories;
	language_directories.emplace_back(utils::resources_directory() / lang_folder);

	auto add_folder = [&language_directories](const std::filesystem::path &file) {
		if (!file.empty()) {
			std::filesystem::path dir = file.parent_path();
			if (std::find(language_directories.begin(), language_directories.end(), dir) == language_directories.end()) {
				language_directories.emplace_back(std::move(dir));
			}
		}
	};

	for (lang_info &info : m_available_langs) {
		add_folder(info.file);
	}

	add_folder(m_user_general_lang.file);
	add_folder(m_user_command_lang.file);
	add_folder(m_user_gui_lang.file);
	add_folder(m_user_log_lang.file);

	if (language_directories.empty()) {
		return;
	}
	m_available_langs.clear();

	namespace fs = std::filesystem;

	for (const fs::path &directory : language_directories) {
		for (const fs::directory_entry &file : fs::directory_iterator{directory}) {
			const fs::path &file_path = file.path();
			if (file.is_regular_file() && file_path.extension() == lang_file_ext) {
				m_available_langs.emplace_back(parse_lang_info<lang_info>(file_path));
			}
		}
	}

	std::sort(m_available_langs.begin(), m_available_langs.end(), [](const lang_info &lhs, const lang_info &rhs) {
		return lhs.name < rhs.name;
	});
}

void resource_manager::refresh_resource_pack_list() {
	m_resource_packs.clear();
	try {
		for (const auto &entry : std::filesystem::directory_iterator{resources_directory() / resource_pack_folder}) {
			if (!entry.is_directory()) {
				continue;
			}
			std::filesystem::path toml_path = entry.path() / resource_pack_toml_name;
			if (std::filesystem::is_regular_file(toml_path)) {
				m_resource_packs.emplace_back(parse_resource_pack_info(toml_path));
			}
		}
	} catch (const std::filesystem::filesystem_error& e) {
		spdlog::error("{}", e.what());
	}
}

void resource_manager::set_user_general_lang(const lang_info &lang) noexcept {
	m_user_general_lang = lang; // todo : sans effet
}

// TODO : mettre les trois "set_user_*_lang" fonctions ensemble
void resource_manager::set_user_command_lang(const lang_info &lang) noexcept {
	try {
		std::unordered_map<command_id, std::pair<std::string, std::string>> str_backup;
		std::swap(str_backup, m_commands_strings);

		std::shared_ptr<cpptoml::table> config = cpptoml::parse_file(lang.file.generic_string());
		if (!config || !load_command_texts(config)) {
			std::swap(str_backup, m_commands_strings);
			spdlog::warn(log_for("resource_manager.commands_texts.reload_failed"));
		}
		m_user_command_lang.file = lang.file;
	}
	catch (const std::exception &e) {
		spdlog::error("{}", e.what());
	}
}

void resource_manager::set_user_gui_lang(const lang_info &lang) noexcept {
	try {
		std::unordered_map<std::string_view, std::string> str_backup;
		std::vector<std::string> keys_backup;
		std::swap(str_backup, m_gui_strings);
		std::swap(keys_backup, m_gui_string_keys);

		std::shared_ptr<cpptoml::table> config = cpptoml::parse_file(lang.file.generic_string());
		if (!config || !load_gui_texts(config)) {
			std::swap(str_backup, m_gui_strings);
			std::swap(keys_backup, m_gui_string_keys);
			spdlog::warn(log_for("resource_manager.gui_texts.reload_failed"));
		}
		m_user_gui_lang.file = lang.file;
	}
	catch (const std::exception &e) {
		spdlog::error("{}", e.what());
	}
}

void resource_manager::set_user_log_lang(const lang_info &lang) noexcept {
	try {
		std::unordered_map<std::string_view, std::string> str_backup;
		std::vector<std::string> keys_backup;
		std::swap(str_backup, m_log_strings);
		std::swap(keys_backup, m_log_string_keys);

		std::shared_ptr<cpptoml::table> config = cpptoml::parse_file(lang.file.generic_string());
		if (!config || !load_logging_texts(config)) {
			std::swap(str_backup, m_log_strings);
			std::swap(keys_backup, m_log_string_keys);
			spdlog::warn(log_for("resource_manger.log_texts.reload_failed"));
		}
		m_user_log_lang.file = lang.file;
	}
	catch (const std::exception &e) {
		spdlog::error("{}", e.what());
	}
}

void resource_manager::set_user_resource_pack(const resource_pack_info &res_pack) noexcept {

	std::unordered_map<std::string, sf::Texture *> textures_by_file_backup{};
	std::forward_list<sf::Texture> textures_holder_backup{};
	std::unordered_map<resources_type::tile_id, view::animation> tiles_anims_backup{};
	std::unordered_map<resources_type::object_id, view::shifted_animation> objects_anims_backup{};
	std::unordered_map<resources_type::mob_id, view::mob_animations> mobs_anims_backup{};
	std::swap(textures_by_file_backup, m_textures_by_file);
	std::swap(textures_holder_backup, m_textures_holder);
	std::swap(tiles_anims_backup, m_tiles_anims);
	std::swap(objects_anims_backup, m_objects_anims);
	std::swap(mobs_anims_backup, m_mobs_anims);

	std::shared_ptr<cpptoml::table> toml = parse_file(res_pack.file);
	if (toml && load_graphics(toml, res_pack.file.parent_path())) {
		m_user_resource_pack = res_pack;
	}
	else {
		std::swap(textures_by_file_backup, m_textures_by_file);
		std::swap(textures_holder_backup, m_textures_holder);
		std::swap(tiles_anims_backup, m_tiles_anims);
		std::swap(objects_anims_backup, m_objects_anims);
		std::swap(mobs_anims_backup, m_mobs_anims);
		spdlog::warn(log_for("resource_manager.resource_pack.reload_failed"));
	}
}

bool resource_manager::save_user_config() const noexcept {
	using namespace fmt::literals;
	namespace uk = config_keys::user;
	namespace uknq = config_keys::user::non_qualified;

	std::filesystem::path config_file = utils::config_directory() / CONFIG_FILE;

    std::shared_ptr<cpptoml::table> lang_config = cpptoml::make_table();
    lang_config->insert(uknq::general_lang, m_user_general_lang.file.generic_string());
    lang_config->insert(uknq::commands_lang, m_user_command_lang.file.generic_string());
    lang_config->insert(uknq::gui_lang, m_user_gui_lang.file.generic_string());
    lang_config->insert(uknq::log_lang, m_user_log_lang.file.generic_string());

	std::shared_ptr<cpptoml::table> config = cpptoml::make_table();
	config->insert(uk::lang_table, lang_config);
	config->insert(config_keys::unqualified_graph_res_file, m_user_resource_pack.file.generic_string());

	std::ofstream ofs(config_file, std::ios_base::trunc | std::ios_base::out);
	if (!ofs) {
		spdlog::error("resource_manager.write_opening_failed", "error"_a = sys_last_error(),
		              "file"_a = (utils::config_directory() / CONFIG_FILE).generic_string());
		return false;
	}

	auto global = cpptoml::make_table();
	global->insert(uk::main_table, config);

	ofs << *global;
	return true;
}

#pragma clang diagnostic pop