#ifndef NINJACLOWN_TERMINAL_IDS_HPP
#define NINJACLOWN_TERMINAL_IDS_HPP

enum class command_id {
    clear               = COMMANDS_CLEARID,
    echo                = COMMANDS_ECHOID,
    exit                = COMMANDS_EXITID,
    help                = COMMANDS_HELPID,
    print               = COMMANDS_PRINTID,
    quit                = COMMANDS_QUITID,
    load_shared_library = COMMANDS_LOAD_DLLID,
    load_map            = COMMANDS_LOAD_MAPID,
    update_world        = COMMANDS_UPDATE_WORLDID,
    set                 = COMMANDS_SETID,
    valueof             = COMMANDS_VALUEOFID,
    reconfigure         = COMMANDS_RELOAD_RESOURCES,
    fire_actionable     = COMMANDS_FIRE_ACTIONABLE,
    fire_activator      = COMMANDS_FIRE_ACTIVATOR,
    OUTOFRANGE
};

enum class variable_id {
    average_fps        = VARIABLES_AVERAGE_FPSID,
    target_fps         = VARIABLES_TARGET_FPSID,
    display_debug_data = VARIABLES_DISPLAY_DEBUG_DATAID,
    OUTOFRANGE
};

#endif //NINJACLOWN_TERMINAL_IDS_HPP
