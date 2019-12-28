message(STATUS "Configuring cpptoml")

get_filename_component(CPPTOML_DIR ${CMAKE_SOURCE_DIR}/external/cpptoml ABSOLUTE)

# Submodule check
missing_external(missing cpptoml)
if(missing)
    message(FATAL_ERROR "cpptoml dependency is missing, maybe you didn't pull the git submodules")
endif()

set(CPPTOML_INCLUDE_DIR ${CPPTOML_DIR}/include)
message(STATUS "Configuring cpptoml - done")
