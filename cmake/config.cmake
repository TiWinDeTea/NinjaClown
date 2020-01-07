if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_compile_definitions(OS_WINDOWS)
    set(CONFIG_OS_WINDOWS 1)
    if (MSVC)
        set(OPTIONS WIN32)
        add_compile_definitions(USE_WINMAIN)
    endif(MSVC)

    set(THREADS_LIBRARIES)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    add_compile_definitions(OS_LINUX)
    set(CONFIG_OS_LINUX 1)
    set(DLL_LOADING_TARGET_LIBRARY dl)

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    set(THREADS_LIBRARIES Threads::Threads)
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


#resources data
set(MOBS_PLAYERID 0)
add_compile_definitions(MOBS_PLAYERID=0)

set(TILES_CHASMID 0)
add_compile_definitions(TILES_CHASMID=0)
set(TILES_IRONID 1)
add_compile_definitions(TILES_IRONID=1)
set(TILES_CONCRETEID 2)
add_compile_definitions(TILES_CONCRETEID=2)
set(TILES_FRAMEID 3)
add_compile_definitions(TILES_FRAMEID=3)

set(OBJECTS_BUTTONID 0)
add_compile_definitions(OBJECTS_BUTTONID=0)
