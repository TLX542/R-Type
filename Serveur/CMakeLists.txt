cmake_minimum_required(VERSION 3.15)

# Check for ASIO
find_path(ASIO_INCLUDE_DIR asio.hpp)

if(NOT ASIO_INCLUDE_DIR)
    # Try to find boost asio as fallback
    find_package(Boost COMPONENTS system)
    if(NOT Boost_FOUND)
        message(FATAL_ERROR "Neither standalone ASIO nor Boost.Asio found. Please install ASIO: sudo apt-get install libasio-dev")
    endif()
endif()

# Server sources
set(SERVER_SOURCES
    src/Server.cpp
    src/Protocol.cpp
    src/GameServer.cpp
    src/main.cpp
)

# Create server executable
add_executable(r-type-server ${SERVER_SOURCES})

# Include directories
target_include_directories(r-type-server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/game
    include
)

# Link game library
target_link_libraries(r-type-server PRIVATE game)

# Link threading
find_package(Threads REQUIRED)
target_link_libraries(r-type-server PRIVATE Threads::Threads)

# Configure ASIO
if(ASIO_INCLUDE_DIR)
    target_include_directories(r-type-server PRIVATE ${ASIO_INCLUDE_DIR})
    target_compile_definitions(r-type-server PRIVATE ASIO_STANDALONE)
else()
    target_include_directories(r-type-server PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(r-type-server PRIVATE ${Boost_LIBRARIES})
endif()

# Compiler settings
target_compile_features(r-type-server PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(r-type-server PRIVATE /W4 /permissive-)
else()
    target_compile_options(r-type-server PRIVATE -Wall -Wextra -Wpedantic)
endif()

# On Linux: copy to repo root for convenience
if(UNIX AND NOT WIN32)
    add_custom_command(TARGET r-type-server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:r-type-server>" "${CMAKE_SOURCE_DIR}/r-type-server"
        COMMAND chmod +x "${CMAKE_SOURCE_DIR}/r-type-server"
        COMMENT "Copying r-type-server executable to repository root"
    )
endif()
