# ============================================================================
# R-Type Cross-Platform CMake Build System
# ============================================================================
# This CMake configuration provides automatic dependency fetching for ASIO
# and raylib, supporting both Linux and Windows platforms.
#
# Features:
# - Automatic dependency download and build via FetchContent
# - Optional system dependency usage
# - vcpkg integration support
# - Cross-platform (Linux, Windows, macOS)
# ============================================================================

cmake_minimum_required(VERSION 3.16)
project(R-Type VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Build Options
# ============================================================================
option(USE_SYSTEM_DEPENDENCIES "Force using system-installed dependencies only" OFF)
option(USE_VCPKG "Enable vcpkg toolchain integration hints" OFF)
option(BUILD_EXAMPLES "Build example executables" OFF)
option(BUILD_TESTS "Build test executables" ON)

# ============================================================================
# Platform Detection and Configuration
# ============================================================================
if(WIN32)
    message(STATUS "Building for Windows")
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 and later
    add_definitions(-DWIN32_LEAN_AND_MEAN)  # Exclude rarely-used Windows headers
    add_definitions(-DNOMINMAX)              # Prevent Windows.h from defining min/max macros
elseif(UNIX AND NOT APPLE)
    message(STATUS "Building for Linux")
elseif(APPLE)
    message(STATUS "Building for macOS")
endif()

# ============================================================================
# vcpkg Integration Hints
# ============================================================================
if(USE_VCPKG AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    message(STATUS "")
    message(STATUS "==================================================")
    message(STATUS "vcpkg Mode Enabled")
    message(STATUS "==================================================")
    message(STATUS "To use vcpkg, specify CMAKE_TOOLCHAIN_FILE:")
    message(STATUS "  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake")
    message(STATUS "")
    if(WIN32)
        message(STATUS "Example for Windows:")
        message(STATUS "  cmake -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ..")
    else()
        message(STATUS "Example for Linux:")
        message(STATUS "  cmake -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake ..")
    endif()
    message(STATUS "==================================================")
    message(STATUS "")
endif()

# ============================================================================
# FetchContent Setup (for automatic dependency download)
# ============================================================================
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ============================================================================
# ASIO Dependency (Standalone)
# ============================================================================
set(ASIO_FOUND FALSE)

# Try system ASIO first if requested
if(USE_SYSTEM_DEPENDENCIES)
    message(STATUS "Searching for system-installed ASIO...")
    find_path(ASIO_INCLUDE_DIR 
        NAMES asio.hpp
        PATHS 
            /usr/include
            /usr/local/include
            ${CMAKE_PREFIX_PATH}/include
    )
    
    if(ASIO_INCLUDE_DIR)
        message(STATUS "Found system ASIO: ${ASIO_INCLUDE_DIR}")
        set(ASIO_FOUND TRUE)
    else()
        message(FATAL_ERROR 
            "USE_SYSTEM_DEPENDENCIES is ON but ASIO not found!\n"
            "Install with: sudo apt-get install libasio-dev (Linux)\n"
            "Or: brew install asio (macOS)\n"
            "Or: vcpkg install asio (Windows/vcpkg)")
    endif()
else()
    # Try to find system ASIO (optional)
    find_path(ASIO_INCLUDE_DIR asio.hpp)
    
    if(ASIO_INCLUDE_DIR)
        message(STATUS "Using system-installed ASIO from: ${ASIO_INCLUDE_DIR}")
        set(ASIO_FOUND TRUE)
    else()
        # Download ASIO via FetchContent
        message(STATUS "ASIO not found on system, downloading via FetchContent...")
        
        FetchContent_Declare(
            asio
            GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
            GIT_TAG asio-1-28-0  # Stable release tag
            GIT_SHALLOW TRUE
        )
        
        FetchContent_MakeAvailable(asio)
        
        # ASIO is header-only, set include directory
        set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include")
        message(STATUS "Downloaded ASIO to: ${ASIO_INCLUDE_DIR}")
        set(ASIO_FOUND TRUE)
    endif()
endif()

# Create ASIO interface library
if(ASIO_FOUND)
    add_library(asio INTERFACE)
    target_include_directories(asio INTERFACE ${ASIO_INCLUDE_DIR})
    target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
    
    # Platform-specific link libraries for ASIO
    if(WIN32)
        target_link_libraries(asio INTERFACE ws2_32 wsock32)
    else()
        # Linux/Unix needs pthread
        find_package(Threads REQUIRED)
        target_link_libraries(asio INTERFACE Threads::Threads)
    endif()
endif()

# ============================================================================
# Raylib Dependency
# ============================================================================
set(RAYLIB_FOUND FALSE)

# Try system raylib first if requested
if(USE_SYSTEM_DEPENDENCIES)
    message(STATUS "Searching for system-installed raylib...")
    find_package(raylib QUIET)
    
    if(raylib_FOUND)
        message(STATUS "Found system raylib")
        set(RAYLIB_FOUND TRUE)
    else()
        message(WARNING 
            "USE_SYSTEM_DEPENDENCIES is ON but raylib not found!\n"
            "render_client will not be built.\n"
            "Install with: sudo apt-get install libraylib-dev (Linux)\n"
            "Or: brew install raylib (macOS)\n"
            "Or: vcpkg install raylib (Windows/vcpkg)")
    endif()
else()
    # Check for local raylib subdirectory first
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/raylib/CMakeLists.txt")
        message(STATUS "Found local raylib directory, building it...")
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        add_subdirectory(raylib)
        set(RAYLIB_FOUND TRUE)
    else()
        # Try to find system-installed raylib
        find_package(raylib QUIET)
        
        if(raylib_FOUND)
            message(STATUS "Using system-installed raylib")
            set(RAYLIB_FOUND TRUE)
        else()
            # Download raylib via FetchContent
            message(STATUS "raylib not found, downloading via FetchContent...")
            
            FetchContent_Declare(
                raylib
                GIT_REPOSITORY https://github.com/raysan5/raylib.git
                GIT_TAG 5.0  # Stable release
                GIT_SHALLOW TRUE
            )
            
            # Configure raylib build options
            set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
            
            FetchContent_MakeAvailable(raylib)
            
            message(STATUS "Downloaded and built raylib")
            set(RAYLIB_FOUND TRUE)
        endif()
    endif()
endif()

# ============================================================================
# Project Include Directories
# ============================================================================
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================================
# Executable Targets
# ============================================================================

# ----------------------------------------------------------------------------
# Server executable (r-type_server)
# ----------------------------------------------------------------------------
set(SERVER_SOURCES
    src/main.cpp
    src/Server.cpp
    src/GameServer.cpp
    src/Protocol.cpp
)

add_executable(r-type_server ${SERVER_SOURCES})
target_link_libraries(r-type_server PRIVATE asio)
target_include_directories(r-type_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ----------------------------------------------------------------------------
# Client test executable (simple TCP client for testing)
# ----------------------------------------------------------------------------
add_executable(client_test src/client_test.cpp)
target_link_libraries(client_test PRIVATE asio)
target_include_directories(client_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ----------------------------------------------------------------------------
# Game client (interactive console client)
# ----------------------------------------------------------------------------
add_executable(game_client
    src/interactive_client.cpp
    src/Protocol.cpp
)
target_link_libraries(game_client PRIVATE asio)
target_include_directories(game_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ----------------------------------------------------------------------------
# Headless test client (automated testing, no graphics)
# ----------------------------------------------------------------------------
if(BUILD_TESTS)
    add_executable(test_headless
        test_headless_client.cpp
        src/GameClient.cpp
        src/Protocol.cpp
    )
    target_link_libraries(test_headless PRIVATE asio)
    target_include_directories(test_headless PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    message(STATUS "test_headless will be built (BUILD_TESTS=ON)")
else()
    message(STATUS "test_headless will NOT be built (BUILD_TESTS=OFF)")
endif()

# ----------------------------------------------------------------------------
# Render client (Raylib graphical client)
# ----------------------------------------------------------------------------
if(RAYLIB_FOUND)
    add_executable(render_client
        src/render_client.cpp
        src/GameClient.cpp
        src/Protocol.cpp
    )
    target_link_libraries(render_client PRIVATE asio raylib)
    target_include_directories(render_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    # On Linux, raylib might need additional libraries
    if(UNIX AND NOT APPLE)
        target_link_libraries(render_client PRIVATE m)  # Math library
    endif()

    message(STATUS "render_client will be built with Raylib support")
else()
    message(STATUS "render_client will NOT be built (Raylib not available)")
endif()

# ============================================================================
# Installation
# ============================================================================
install(TARGETS r-type_server client_test game_client DESTINATION bin)

if(BUILD_TESTS)
    install(TARGETS test_headless DESTINATION bin)
endif()

if(RAYLIB_FOUND)
    install(TARGETS render_client DESTINATION bin)
endif()

# ============================================================================
# Build Configuration Summary
# ============================================================================
message(STATUS "")
message(STATUS "========================================================")
message(STATUS "R-Type Build Configuration Summary")
message(STATUS "========================================================")
message(STATUS "CMake Version:              ${CMAKE_VERSION}")
message(STATUS "Build Type:                 ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:               C++${CMAKE_CXX_STANDARD}")
message(STATUS "System:                     ${CMAKE_SYSTEM_NAME}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  USE_SYSTEM_DEPENDENCIES:  ${USE_SYSTEM_DEPENDENCIES}")
message(STATUS "  USE_VCPKG:                ${USE_VCPKG}")
message(STATUS "  BUILD_EXAMPLES:           ${BUILD_EXAMPLES}")
message(STATUS "  BUILD_TESTS:              ${BUILD_TESTS}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  ASIO:                     ${ASIO_FOUND}")
if(ASIO_FOUND)
    message(STATUS "    Include Dir:          ${ASIO_INCLUDE_DIR}")
endif()
message(STATUS "  Raylib:                   ${RAYLIB_FOUND}")
message(STATUS "")
message(STATUS "Targets to Build:")
message(STATUS "  r-type_server:            YES")
message(STATUS "  client_test:              YES")
message(STATUS "  game_client:              YES")
message(STATUS "  render_client:            ${RAYLIB_FOUND}")
message(STATUS "  test_headless:            ${BUILD_TESTS}")
message(STATUS "")
message(STATUS "========================================================")
message(STATUS "")

if(NOT RAYLIB_FOUND)
    message(STATUS "NOTE: render_client will not be built because raylib is not available.")
    message(STATUS "      To enable it, either:")
    message(STATUS "      - Install raylib system-wide")
    message(STATUS "      - Set USE_SYSTEM_DEPENDENCIES=OFF to auto-download")
    message(STATUS "")
endif()

# ============================================================================
# Helpful Build Commands
# ============================================================================
if(CMAKE_GENERATOR MATCHES "Unix Makefiles")
    message(STATUS "Build commands:")
    message(STATUS "  make              - Build all targets")
    message(STATUS "  make -j<N>        - Build with N parallel jobs")
    message(STATUS "  make -j\${nproc}  - Build with all available cores")
    message(STATUS "")
endif()
