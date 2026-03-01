cmake_minimum_required(VERSION 3.13)
project(uc-online VERSION 1.0.0 LANGUAGES C RC)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# ============================================================================
# 32-bit DLL (steam_api.dll proxy)
# ============================================================================

add_library(steam_api SHARED
    src/dllmain.c
    src/steam_api_stubs.c
    src/uc_online.c
    src/ini_config.c
    src/logger.c
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
        # MinGW / GCC linker: pass the .def file as a positional argument.
        # IMPORTANT: use the MINGW32 (i686) toolchain for 32-bit builds.
        # Do NOT use the MINGW64 toolchain with -m32; it ships only 64-bit libs.
        #
        # -static-libgcc statically links the GCC runtime.
        # -s strips debug symbols from the output DLL.
        target_link_options(steam_api PRIVATE
            -static
            -static-libgcc
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
    src/dllmain.c
    src/steam_api_stubs.c
    src/uc_online.c
    src/ini_config.c
    src/logger.c
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
            target_link_libraries(steam_api64 PRIVATE kernel32)

            target_link_options(steam_api64 PRIVATE
                -static
                -static-libgcc
                -Wl,--as-needed
                -Wl,--gc-sections
                "${CMAKE_SOURCE_DIR}/src/steam_api64_proxy.def"
            )
        endif()
    endif()
endif()

# Copy config.ini if it exists
if(EXISTS ${CMAKE_SOURCE_DIR}/config.ini)
    configure_file(config.ini config.ini COPYONLY)
endif()
