message(STATUS "Configuring imterm")

get_filename_component(IMTERM_DIR ${CMAKE_SOURCE_DIR}/external/ImTerm ABSOLUTE)

# Submodule check
missing_external(missing ImTerm)
if(missing)
    message(FATAL_ERROR "imterm dependency is missing, maybe you didn't pull the git submodules")
endif()

add_subdirectory(${IMTERM_DIR})
set(IMTERM_INCLUDE_DIR ${IMTERM_DIR}/include)
message(STATUS "Configuring imterm - done")
