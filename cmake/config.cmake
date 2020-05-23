if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_compile_definitions(OS_WINDOWS)
    add_compile_definitions(NOMINMAX)

    set(CONFIG_OS_WINDOWS 1)
    if (MSVC)
        set(OPTIONS WIN32)
        add_compile_definitions(USE_WINMAIN)
    endif(MSVC)

    set(THREADS_LIBRARIES)
    set(FILESYSTEM_LIBRARIES)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_compile_definitions(OS_LINUX)
    set(CONFIG_OS_LINUX 1)
    set(DLL_LOADING_TARGET_LIBRARY dl)

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    set(THREADS_LIBRARIES Threads::Threads)
    set(FILESYSTEM_LIBRARIES stdc++fs)
else()
    message(FATAL_ERROR "unsupported OS")

endif()

if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(CONFIG_ARCH_32 1)
elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(CONFIG_ARCH_64 1)
endif()

add_compile_definitions(IMTERM_ENABLE_REGEX)
add_compile_definitions(IMTERM_USE_FMT)
add_compile_definitions(FMT_HEADER_ONLY)
set(COMPILE_SFML_WITH_PROJECT OFF) # Use system SFML if present
set(SFML_MINIMUM_SYSTEM_VERSION 2.5)

##################################################################################################
########################################                  ########################################
########################################  resources data  ########################################
########################################                  ########################################
##################################################################################################

# mobs
set(MOBS_PLAYERID 0)
add_compile_definitions(MOBS_PLAYERID=0)
set(MOBS_SCIENTISTID 1)
add_compile_definitions(MOBS_SCIENTISTID=1)

# tiles
set(TILES_CHASMID 0)
add_compile_definitions(TILES_CHASMID=0)
set(TILES_IRONID 1)
add_compile_definitions(TILES_IRONID=1)
set(TILES_CONCRETEID 2)
add_compile_definitions(TILES_CONCRETEID=2)
set(TILES_FRAMEID 3)
add_compile_definitions(TILES_FRAMEID=3)

# objects
set(OBJECTS_BUTTONID 0)
add_compile_definitions(OBJECTS_BUTTONID=0)
set(OBJECTS_GATEID 1)
add_compile_definitions(OBJECTS_GATEID=1)
set(OBJECTS_AUTOSHOOTERID 2)
add_compile_definitions(OBJECTS_AUTOSHOOTERID=2)
set(OBJECTS_TARGETID 3)
add_compile_definitions(OBJECTS_TARGETID=3)

# command ids
set(COMMANDS_CLEARID 0)
add_compile_definitions(COMMANDS_CLEARID=0)
set(COMMANDS_ECHOID 1)
add_compile_definitions(COMMANDS_ECHOID=1)
set(COMMANDS_EXITID 2)
add_compile_definitions(COMMANDS_EXITID=2)
set(COMMANDS_HELPID 3)
add_compile_definitions(COMMANDS_HELPID=3)
set(COMMANDS_PRINTID 4)
add_compile_definitions(COMMANDS_PRINTID=4)
set(COMMANDS_QUITID 5)
add_compile_definitions(COMMANDS_QUITID=5)
set(COMMANDS_LOAD_DLLID 6)
add_compile_definitions(COMMANDS_LOAD_DLLID=6)
set(COMMANDS_LOAD_MAPID 7)
add_compile_definitions(COMMANDS_LOAD_MAPID=7)
set(COMMANDS_UPDATE_WORLDID 8)
add_compile_definitions(COMMANDS_UPDATE_WORLDID=8)
set(COMMANDS_SETID 9)
add_compile_definitions(COMMANDS_SETID=9)
set(COMMANDS_VALUEOFID 10)
add_compile_definitions(COMMANDS_VALUEOFID=10)
set(COMMANDS_RELOAD_RESOURCES 11)
add_compile_definitions(COMMANDS_RELOAD_RESOURCES=11)
set(COMMANDS_FIRE_ACTIONABLE 12)
add_compile_definitions(COMMANDS_FIRE_ACTIONABLE=12)
set(COMMANDS_FIRE_ACTIVATOR 13)
add_compile_definitions(COMMANDS_FIRE_ACTIVATOR=13)

# variable names
set(VARIABLES_AVERAGE_FPSID 0)
add_compile_definitions(VARIABLES_AVERAGE_FPSID=0)
set(VARIABLES_TARGET_FPSID 1)
add_compile_definitions(VARIABLES_TARGET_FPSID=1)
set(VARIABLES_DISPLAY_DEBUG_DATAID 2)
add_compile_definitions(VARIABLES_DISPLAY_DEBUG_DATAID=2)
