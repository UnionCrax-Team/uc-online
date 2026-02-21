cmake_minimum_required(VERSION 3.10)
project(uc-online VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# 32-bit DLL version (steam_api.dll proxy)
add_library(steam_api SHARED
    src/dllmain.cpp
    src/steam_api_stubs.cpp
    src/uc_online.cpp
    src/ini_config.cpp
    src/logger.cpp
    src/resources.rc
)

# For Windows, link kernel32 for LoadLibrary, etc.
if(WIN32)
    target_link_libraries(steam_api PRIVATE kernel32)
    target_compile_definitions(steam_api PRIVATE IS_32BIT UC_ONLINE_EXPORTS)
    # Use .def file for steam_api proxy exports
    set_target_properties(steam_api PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/src/steam_api_proxy.def"
    )
    # Force 32-bit compilation
    if(MSVC)
        set_target_properties(steam_api PROPERTIES
            COMPILE_FLAGS "/arch:IA32"
        )
    endif()
endif()

# 64-bit DLL version (steam_api64.dll proxy)
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
    # Use .def file for steam_api64 proxy exports
    set_target_properties(steam_api64 PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/src/steam_api64_proxy.def /MACHINE:X64"
    )
endif()

# Copy config.ini if it exists
if(EXISTS ${CMAKE_SOURCE_DIR}/config.ini)
    configure_file(config.ini config.ini COPYONLY)
endif()
