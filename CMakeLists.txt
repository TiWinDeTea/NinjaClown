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
		src/dll.cpp

		src/view/map.cpp
		src/view/viewer.cpp
)
set_target_properties(
	ninja-clown PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)
target_include_directories(ninja-clown PUBLIC include/)
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
)

file(GLOB_RECURSE files "resources/*")
foreach(file ${files})
    file(RELATIVE_PATH relative_file resources/ ${file})
    configure_file(${file} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/${relative_file}" @ONLY)
endforeach()
