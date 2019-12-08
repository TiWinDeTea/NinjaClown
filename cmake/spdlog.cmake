message(STATUS "Configuring spdlog")

get_filename_component(SPDLOG_DIR ${CMAKE_SOURCE_DIR}/external/spdlog ABSOLUTE)

# Submodule check
missing_external(missing "${SPDLOG_DIR}")
if(missing)
    message(FATAL_ERROR "Spdlog dependency is missing, maybe you didn't pull the git submodules")
endif()

# Variables
get_filename_component(SPDLOG_INCLUDE_DIR  ${SPDLOG_DIR}/include  ABSOLUTE)
message(STATUS "Configuring spdlog - Done")