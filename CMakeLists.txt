cmake_minimum_required(VERSION 3.10)
project(NinjaClown)

include(cmake/common.cmake)
include(cmake/config.cmake)

include(cmake/imgui-sfml.cmake)
include(cmake/imterm.cmake)
include(cmake/spdlog.cmake)
include(cmake/fmt.cmake)
include(cmake/cpptoml.cmake)

add_executable(ninja-clown ${OPTIONS}
		src/main.cpp
		src/terminal_commands.cpp
		src/state_holder.cpp

		src/adapter/adapter.cpp

		src/bot/bot_api.cpp
		src/bot/bot_dll.cpp

		src/model/button.cpp
		src/model/world.cpp
		src/model/model.cpp

		src/utils/dll.cpp
		src/utils/resource_manager.cpp

		src/view/animation.cpp
		src/view/map.cpp
		src/view/mob_animations.cpp
		src/view/overmap_collection.cpp
		src/view/viewer.cpp
)

set_target_properties(
	ninja-clown PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
target_include_directories(ninja-clown PUBLIC include/ ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(ninja-clown SYSTEM PUBLIC
        ${IMGUI_SFML_INCLUDE_DIR}
        ${IMTERM_INCLUDE_DIR}
        ${SPDLOG_INCLUDE_DIR}
        ${SFML_INCLUDE_DIR}
		${FMT_INCLUDE_DIR}
		${CPPTOML_INCLUDE_DIR}
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
foreach(file ${files})
	message(STATUS "resources/configured_resources/${file}   ->   resources/${file}")
	configure_file("resources/configured_resources/${file}" "resources/${file}" @ONLY)
endforeach()

file(GLOB_RECURSE files RELATIVE "${CMAKE_CURRENT_LIST_DIR}/resources/raw_resources/" "resources/raw_resources/*")
foreach(file ${files})
	message(STATUS "resources/raw_resources/${file}   ->   resources/${file}")
	configure_file("resources/raw_resources/${file}" "resources/${file}" COPYONLY)
endforeach()


add_library(ninja-clown-bot MODULE bots/test.c)
target_include_directories(ninja-clown-bot SYSTEM PRIVATE ${CMAKE_CURRENT_LIST_DIR})

add_dependencies(ninja-clown ninja-clown-bot)
