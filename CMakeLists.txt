cmake_minimum_required(VERSION 3.17)
project(RType)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    message(STATUS "Building for Windows")
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 and later
elseif(UNIX)
    message(STATUS "Building for Unix/Linux")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# ============================================================================
# Find ASIO (standalone or Boost.Asio)
# ============================================================================
find_package(Boost COMPONENTS system QUIET)

if(Boost_FOUND)
    message(STATUS "Using Boost.Asio")
    include_directories(${Boost_INCLUDE_DIRS})
    set(ASIO_LIBS ${Boost_LIBRARIES})
else()
    # Try standalone ASIO
    message(STATUS "Boost not found, looking for standalone ASIO")
    find_path(ASIO_INCLUDE_DIR asio.hpp)
    if(ASIO_INCLUDE_DIR)
        message(STATUS "Using standalone ASIO from: ${ASIO_INCLUDE_DIR}")
        include_directories(${ASIO_INCLUDE_DIR})
        add_definitions(-DASIO_STANDALONE)
        if(WIN32)
            set(ASIO_LIBS ws2_32 wsock32)  # Windows socket libraries
        else()
            set(ASIO_LIBS pthread)
        endif()
    else()
        message(FATAL_ERROR "Neither Boost.Asio nor standalone ASIO found. Install with: sudo apt-get install libasio-dev")
    endif()
endif()

# ============================================================================
# Find or build Raylib
# ============================================================================
# First, try to find raylib in the local raylib/ directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/raylib/CMakeLists.txt")
    message(STATUS "Found local raylib directory, using it")
    option(BUILD_EXAMPLES "Build the examples" OFF)
    add_subdirectory(raylib)
    set(RAYLIB_FOUND TRUE)
else()
    # Try to find system-installed raylib
    find_package(raylib QUIET)
    if(raylib_FOUND)
        message(STATUS "Using system-installed raylib")
        set(RAYLIB_FOUND TRUE)
    else()
        message(WARNING "Raylib not found. render_client will not be built.")
        message(WARNING "To fix: Clone raylib into ${CMAKE_CURRENT_SOURCE_DIR}/raylib/ or install system-wide")
        set(RAYLIB_FOUND FALSE)
    endif()
endif()

# ============================================================================
# Server executable
# ============================================================================
set(SERVER_SOURCES
    src/main.cpp
    src/Server.cpp
    src/GameServer.cpp
    src/Protocol.cpp
)

add_executable(r-type_server ${SERVER_SOURCES})
target_link_libraries(r-type_server ${ASIO_LIBS})

# ============================================================================
# Client test executable (simple TCP client)
# ============================================================================
add_executable(client_test src/client_test.cpp)
target_link_libraries(client_test ${ASIO_LIBS})

# ============================================================================
# Game client (interactive console client)
# ============================================================================
add_executable(game_client
    src/interactive_client.cpp
    src/Protocol.cpp
)
target_link_libraries(game_client ${ASIO_LIBS})

# ============================================================================
# Render client (Raylib graphical client)
# ============================================================================
if(RAYLIB_FOUND)
    add_executable(render_client
        src/render_client.cpp
        src/GameClient.cpp
        src/Protocol.cpp
    )
    target_link_libraries(render_client ${ASIO_LIBS} raylib)

    # On Linux, raylib might need additional libraries
    if(UNIX AND NOT APPLE)
        target_link_libraries(render_client m)  # Math library
    endif()

    message(STATUS "render_client will be built with Raylib support")
else()
    message(STATUS "render_client will NOT be built (Raylib not found)")
endif()

# ============================================================================
# Headless test client (automated testing, no graphics)
# ============================================================================
add_executable(test_headless
    test_headless_client.cpp
    src/GameClient.cpp
    src/Protocol.cpp
)
target_link_libraries(test_headless ${ASIO_LIBS})

# ============================================================================
# Installation
# ============================================================================
install(TARGETS r-type_server client_test game_client test_headless DESTINATION bin)

if(RAYLIB_FOUND)
    install(TARGETS render_client DESTINATION bin)
endif()

# ============================================================================
# Print build summary
# ============================================================================
message(STATUS "")
message(STATUS "==================================================")
message(STATUS "R-Type Build Configuration Summary")
message(STATUS "==================================================")
message(STATUS "Server:        r-type_server")
message(STATUS "Test Client:   client_test")
message(STATUS "Game Client:   game_client")
message(STATUS "Render Client: ${RAYLIB_FOUND}")
message(STATUS "Headless Test: test_headless")
message(STATUS "==================================================")
message(STATUS "")
