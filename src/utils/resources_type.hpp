#ifndef NINJACLOWN_UTILS_RESOURCES_TYPE_HPP
#define NINJACLOWN_UTILS_RESOURCES_TYPE_HPP

namespace utils::resources_type {
enum class mob_id {
	dll        = MOBS_DLLID,
	harmless   = MOBS_HARMLESSID,
	patrol     = MOBS_PATROLID,
	aggressive = MOBS_AGGRESSIVEID,

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
	frame    = TILES_FRAMEID, // TODO What is this one all about ?
};
} // namespace utils::resources_type

#endif //NINJACLOWN_UTILS_RESOURCES_TYPE_HPP
