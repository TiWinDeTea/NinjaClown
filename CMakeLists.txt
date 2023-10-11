cmake_minimum_required(VERSION 3.22)
project(NinjaClown)

include(cmake/common.cmake)
include(cmake/config.cmake)

include(cmake/imgui-sfml.cmake)
include(cmake/imterm.cmake)
include(cmake/spdlog.cmake)
include(cmake/fmt.cmake)
include(cmake/cpptoml.cmake)

set(NINJA_CLOWN_SOURCES
        src/terminal_commands.cpp
        src/state_holder.cpp

        src/adapter/adapter.cpp
        src/adapter/adapter_map_loader_v1_0_0.cpp
        src/adapter/facing_dir.cpp

        src/bot/bot_api.cpp
        src/bot/bot_dll.cpp

        src/model/actionable.cpp
        src/model/collision.cpp
        src/model/world.cpp
        src/model/vec2.cpp
        src/model/model.cpp
        src/model/event.cpp

        src/utils/dll.cpp
        src/utils/logging.cpp
        src/utils/loop_per_sec_limit.cpp
        src/utils/resource_manager.cpp
        src/utils/system.cpp

        src/view/assets/animation.cpp
        src/view/game/game_viewer.cpp
        src/view/game/game_menu.cpp
        src/view/game/map.cpp
        src/view/game/map_viewer.cpp
        src/view/game/mob.cpp
        src/view/game/object.cpp
        src/view/game/overmap_collection.cpp
        src/view/standalones/configurator.cpp
        src/view/standalones/file_explorer.cpp
        src/view/standalones/imgui_styles.cpp
        src/view/view.cpp
)

add_executable(ninja-clown ${OPTIONS} ${NINJA_CLOWN_SOURCES} src/main.cpp)

set_target_properties(
        ninja-clown PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

#target_compile_options(ninja-clown PRIVATE "-fsanitize=address")
#target_link_options(ninja-clown PRIVATE "-fsanitize=address")

target_include_directories(ninja-clown PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/ ${CMAKE_CURRENT_LIST_DIR}/bindings/c/)
target_include_directories(ninja-clown SYSTEM PUBLIC
        ${IMGUI_SFML_INCLUDE_DIR}
        ${IMTERM_INCLUDE_DIR}
        ${SPDLOG_INCLUDE_DIR}
        ${SFML_INCLUDE_DIR}
        ${FMT_INCLUDE_DIR}
        ${CPPTOML_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/external/
)

target_link_libraries(
        ninja-clown
        ${IMGUI_SFML_LIBRARIES}
        ${SFML_LIBRARIES}
        ${DLL_LOADING_TARGET_LIBRARY}
        ${THREADS_LIBRARIES}
        ${FILESYSTEM_LIBRARIES}
)

file(GLOB_RECURSE files RELATIVE "${CMAKE_CURRENT_LIST_DIR}/resources/configured_resources/" "resources/configured_resources/*")
foreach (file ${files})
    message(STATUS "resources/configured_resources/${file}   ->   resources/${file}")
    configure_file("resources/configured_resources/${file}" "resources/${file}" @ONLY)
endforeach ()

file(GLOB_RECURSE files RELATIVE "${CMAKE_CURRENT_LIST_DIR}/resources/raw_resources/" "resources/raw_resources/*")
foreach (file ${files})
    message(STATUS "resources/raw_resources/${file}   ->   resources/${file}")
    configure_file("resources/raw_resources/${file}" "resources/${file}" COPYONLY)
endforeach ()


add_library(ninja-clown-basic-bot MODULE examples/basic.c)
target_include_directories(ninja-clown-basic-bot SYSTEM PRIVATE ${CMAKE_CURRENT_LIST_DIR}/bindings/c/)

add_dependencies(ninja-clown ninja-clown-basic-bot)

# tests

set(NINJA_CLOWN_TESTS_SOURCES
        tests/collisions.cpp
)

add_executable(ninja-clown-tests ${NINJA_CLOWN_SOURCES} ${NINJA_CLOWN_TESTS_SOURCES} tests/main.cpp)

set_target_properties(
        ninja-clown-tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

target_include_directories(ninja-clown-tests SYSTEM PUBLIC
        ${IMGUI_SFML_INCLUDE_DIR}
        ${IMTERM_INCLUDE_DIR}
        ${SPDLOG_INCLUDE_DIR}
        ${SFML_INCLUDE_DIR}
        ${FMT_INCLUDE_DIR}
        ${CPPTOML_INCLUDE_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/bindings/c/
        ${CMAKE_CURRENT_LIST_DIR}/external/
        ${CMAKE_CURRENT_LIST_DIR}/src/
)

target_link_libraries(
        ninja-clown-tests
        ${IMGUI_SFML_LIBRARIES}
        ${SFML_LIBRARIES}
        ${DLL_LOADING_TARGET_LIBRARY}
        ${THREADS_LIBRARIES}
        ${FILESYSTEM_LIBRARIES}
)
