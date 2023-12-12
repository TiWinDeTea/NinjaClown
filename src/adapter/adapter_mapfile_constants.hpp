#ifndef NINJACLOWN_ADAPTER_MAPFILE_CONSTANTS_HPP
#define NINJACLOWN_ADAPTER_MAPFILE_CONSTANTS_HPP


namespace adapter::map_file::keys {
constexpr const char *name                  = "name";
constexpr const char *hp                    = "hp";
constexpr const char *attack_delay          = "attack_delay";
constexpr const char *throw_delay           = "throw_delay";
constexpr const char *behaviour             = "behaviour";
constexpr const char *type                  = "type";
constexpr const char *x_pos                 = "pos.x";
constexpr const char *y_pos                 = "pos.y";
constexpr const char *facing                = "facing";
constexpr const char *firing_rate           = "firing_rate";
constexpr const char *closed                = "closed";
constexpr const char *refire_after          = "refire_after";
constexpr const char *refire_repeat         = "refire_repeat";
constexpr const char *activation_difficulty = "activation_difficulty";
constexpr const char *duration              = "duration";
constexpr const char *delay                 = "delay";
constexpr const char *acts_on_table         = "acts_on";
constexpr const char *mobs_spawn_table      = "mobs.spawn";
constexpr const char *actors_spawn_table    = "actors.spawn";
constexpr const char *layout                = "layout";
constexpr const char *map_layout            = "map.layout";
constexpr const char *target                = "target.location";
} // namespace keys

namespace adapter::map_file::values {
constexpr const char *none           = "none";
constexpr const char *harmless       = "harmless";
constexpr const char *patrol         = "patrol";
constexpr const char *aggressive     = "aggressive";
constexpr const char *dll            = "dll";
constexpr const char *button         = "button";
constexpr const char *induction_loop = "induction_loop";
constexpr const char *infrared_laser = "infrared_laser";
constexpr const char *autoshooter    = "autoshooter";
constexpr const char *gate           = "gate";
constexpr const char *unknown        = "unknown";
} // namespace values


#endif //NINJACLOWN_ADAPTER_MAPFILE_CONSTANTS_HPP
