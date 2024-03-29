#ifndef NINJACLOWN_UTILS_RESOURCES_TYPE_HPP
#define NINJACLOWN_UTILS_RESOURCES_TYPE_HPP

namespace utils::resources_type {
enum class mob_id {
	player    = MOBS_PLAYERID,
	scientist = MOBS_SCIENTISTID,
};

enum class object_id {
	button      = OBJECTS_BUTTONID,
	gate        = OBJECTS_GATEID,
	autoshooter = OBJECTS_AUTOSHOOTERID,
	target      = OBJECTS_TARGETID,
};

enum class tile_id {
	chasm    = TILES_CHASMID,
	iron     = TILES_IRONID,
	concrete = TILES_CONCRETEID,
	frame    = TILES_FRAMEID, // used to highlight tiles targeted by actionnable objects 
};
} // namespace utils::resources_type

#endif //NINJACLOWN_UTILS_RESOURCES_TYPE_HPP
