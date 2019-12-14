message(STATUS "Configuring fmt")

get_filename_component(FMT_DIR ${CMAKE_SOURCE_DIR}/external/fmt ABSOLUTE)

# Submodule check
missing_external(missing "${FMT_DIR}")
if(missing)
    message(FATAL_ERROR "Fmt dependency is missing, maybe you didn't pull the git submodules")
endif()

add_subdirectory(${FMT_DIR} EXCLUDE_FROM_ALL)
set(FMT_INCLUDE_DIR  ${FMT_DIR}/include)
message(STATUS "Configuring fmt - Done")
