cmake_minimum_required(VERSION 3.13)
project(uc-online VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# ============================================================================
# 32-bit DLL (steam_api.dll proxy)
# ============================================================================

add_library(steam_api SHARED
    src/dllmain.cpp
    src/steam_api_stubs.cpp
    src/uc_online.cpp
    src/ini_config.cpp
    src/logger.cpp
    src/resources.rc
)

if(WIN32)
    target_link_libraries(steam_api PRIVATE kernel32)
    target_compile_definitions(steam_api PRIVATE IS_32BIT UC_ONLINE_EXPORTS)

    if(MSVC)
        # MSVC linker: /DEF:<path>
        # /arch:IA32 is only meaningful for x86 MSVC; it is set via the -A Win32
        # generator flag when invoking CMake, so we only need the .def here.
        set_target_properties(steam_api PROPERTIES
            LINK_FLAGS "/DEF:\"${CMAKE_SOURCE_DIR}/src/steam_api_proxy.def\""
        )
    else()

    # MinGW / GCC linker (fully static runtime)
    if(MINGW)

        target_link_libraries(steam_api PRIVATE
            kernel32
            winpthread
        )

        target_link_options(steam_api PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
            -Wl,--as-needed
            -Wl,--gc-sections
            "${CMAKE_SOURCE_DIR}/src/steam_api_proxy.def"
        )

    endif()

endif()
# ============================================================================
# 64-bit DLL (steam_api64.dll proxy)
# ============================================================================

add_library(steam_api64 SHARED
    src/dllmain64.cpp
    src/steam_api_stubs.cpp
    src/uc_online64.cpp
    src/ini_config.cpp
    src/logger.cpp
    src/resources.rc
)

if(WIN32)
    target_link_libraries(steam_api64 PRIVATE kernel32)
    target_compile_definitions(steam_api64 PRIVATE IS_64BIT UC_ONLINE_EXPORTS)

    if(MSVC)
        # MSVC linker: /DEF:<path> /MACHINE:X64
        set_target_properties(steam_api64 PROPERTIES
            LINK_FLAGS "/DEF:\"${CMAKE_SOURCE_DIR}/src/steam_api64_proxy.def\" /MACHINE:X64"
        )
   else()

    # MinGW / GCC linker (fully static runtime)
    if(MINGW)

        target_link_libraries(steam_api64 PRIVATE
            kernel32
            winpthread
        )

        target_link_options(steam_api64 PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
            -Wl,--as-needed
            -Wl,--gc-sections
            "${CMAKE_SOURCE_DIR}/src/steam_api64_proxy.def"
        )

    endif()

endif()

# Copy config.ini if it exists
if(EXISTS ${CMAKE_SOURCE_DIR}/config.ini)
    configure_file(config.ini config.ini COPYONLY)
endif()
