message(STATUS "Configuring OpenGL")

set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL REQUIRED)

if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "OpenGL not found")
    return()
endif()

set(OPENGL_INCLUDE_DIR   ${OPENGL_INCLUDE_DIRS})
set(OPENGL_LIBRARIES       ${OPENGL_gl_LIBRARY})

# Message
message(STATUS "Configuring OpenGL - Done")

message(STATUS "Configuring SFML")

if(NOT COMPILE_SFML_WITH_PROJECT)
    if(CONFIG_OS_WINDOWS)
        set(COMPILE_SFML_WITH_PROJECT ON)
        message(STATUS "OS is Windows, SFML will be compiled with project")
    else()
        find_package(SFML ${SFML_MINIMUM_SYSTEM_VERSION} COMPONENTS system window graphics audio CONFIG)
        if(SFML_FOUND)
            # Variables
            set(SFML_INCLUDE_DIR  "")
            set(SFML_LIBRARIES sfml-system sfml-window sfml-graphics sfml-audio)
            message(STATUS "Configuring SFML - done")
        else()
            set(COMPILE_SFML_WITH_PROJECT ON)
            message(STATUS "SFML system installation not found, compile SFML with project")
        endif()
    endif()
endif()

if(COMPILE_SFML_WITH_PROJECT)
    get_filename_component(SFML_DIR ${CMAKE_SOURCE_DIR}/external/SFML ABSOLUTE)

    # Submodule check
    missing_external(missing SFML)
    if(missing)
        message(FATAL_ERROR "SFML dependency is missing, maybe you didn't pull the git submodules")
    endif()

    # Subproject
    add_subdirectory(${SFML_DIR})

    # Configure SFML folder in IDE
    foreach(sfml_target IN ITEMS sfml-system sfml-network sfml-window sfml-graphics sfml-audio sfml-main)
        if(TARGET ${sfml_target})
            set_target_properties(${sfml_target} PROPERTIES FOLDER external/SFML)
        endif()
    endforeach()

    # Configure OpenAL
    if(CONFIG_OS_WINDOWS)
        set(ARCH_FOLDER "x86")
        if(CONFIG_ARCH_64)
            set(ARCH_FOLDER "x64")
        endif()
        configure_file(${SFML_DIR}/extlibs/bin/${ARCH_FOLDER}/openal32.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COPYONLY)
    endif()

    # Setup targets output, put exe and required SFML dll in the same folder
    target_set_output_directory(sfml-system "${CMAKE_BINARY_DIR}")
    target_set_output_directory(sfml-window "${CMAKE_BINARY_DIR}")
    target_set_output_directory(sfml-graphics "${CMAKE_BINARY_DIR}")
    target_set_output_directory(sfml-audio "${CMAKE_BINARY_DIR}")

    get_filename_component(SFML_INCLUDE_DIR  ${SFML_DIR}/include  ABSOLUTE)
    set(SFML_LIBRARIES sfml-system sfml-window sfml-graphics sfml-audio)
    message(STATUS "Configuring SFML - Done")
endif()

########################################################################################################################

message(STATUS "Configuring imgui-sfml")

get_filename_component(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/ImGui ABSOLUTE)
get_filename_component(IMGUI_SFML_DIR ${CMAKE_SOURCE_DIR}/external/ImGui-SFML ABSOLUTE)
get_filename_component(IMGUI_SFML_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR}/ImGui-SFML ABSOLUTE)

# Submodules check
missing_external(missing ImGui)
if(missing)
    message(FATAL_ERROR "ImGui dependency is missing, maybe you didn't pull the git submodules")
endif()
missing_external(missing ImGui-SFML)
if(missing)
    message(FATAL_ERROR "imgui-sfml dependency is missing, maybe you didn't pull the git submodules")
endif()

# Copy imgui and imgui-sfml files to cmake build folder
configure_folder(${IMGUI_DIR} ${IMGUI_SFML_TARGET_DIR} COPYONLY)
configure_folder(${IMGUI_SFML_DIR} ${IMGUI_SFML_TARGET_DIR} COPYONLY)

# Include imgui-sfml config header in imgui config header
file(APPEND "${IMGUI_SFML_TARGET_DIR}/imconfig.h" "\n#include \"imconfig-SFML.h\"\n")

# Setup target
get_files(files "${IMGUI_SFML_TARGET_DIR}")
add_library(imgui-sfml STATIC ${files})
target_include_directories(imgui-sfml SYSTEM PRIVATE "${SFML_INCLUDE_DIR}" "${IMGUI_SFML_TARGET_DIR}")

target_link_libraries(imgui-sfml PRIVATE "${SFML_LIBRARIES}" "${OPENGL_LIBRARIES}")
target_compile_definitions(imgui-sfml PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

# Variables
get_filename_component(IMGUI_SFML_INCLUDE_DIR  "${IMGUI_SFML_TARGET_DIR}"  ABSOLUTE)
set(IMGUI_SFML_LIBRARIES imgui-sfml)

# Message
message(STATUS "Configuring imgui-sfml - Done")