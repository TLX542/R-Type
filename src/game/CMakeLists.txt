cmake_minimum_required(VERSION 3.15)

# Collect game library source files
set(GAME_SOURCES
    Game.cpp
)

set(GAME_HEADERS
    Game.hpp
)

# Create static library for game logic
add_library(game STATIC ${GAME_SOURCES} ${GAME_HEADERS})

# Include directories
target_include_directories(game PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set C++17 standard
target_compile_features(game PUBLIC cxx_std_17)

# Compiler warnings
if (MSVC)
    target_compile_options(game PRIVATE /W4 /permissive-)
else()
    target_compile_options(game PRIVATE -Wall -Wextra -Wpedantic)
endif()
