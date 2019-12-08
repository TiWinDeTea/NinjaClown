if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_compile_definitions(OS_WINDOWS)
    set(CONFIG_OS_WINDOWS 1)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_compile_definitions(OS_LINUX)
    set(CONFIG_OS_LINUX 1)

else()
    message(FATAL_ERROR "unsupported OS")

endif()

if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    set(CONFIG_ARCH_32 1)
elseif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(CONFIG_ARCH_64 1)
endif()

add_compile_definitions(IMTERM_ENABLE_REGEX)
set(COMPILE_SFML_WITH_PROJECT OFF) # Use system SFML if present
set(SFML_MINIMUM_SYSTEM_VERSION 2.5)
