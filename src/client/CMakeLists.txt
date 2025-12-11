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

# Client sources
set(CLIENT_SOURCES
    ClientMain.cpp
)

# Create client executable
add_executable(r-type-client ${CLIENT_SOURCES})

# Include directories
target_include_directories(r-type-client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/Serveur/include
)

# Link SFML
target_link_libraries(r-type-client PRIVATE sfml-graphics sfml-window sfml-system)

# Link threading
find_package(Threads REQUIRED)
target_link_libraries(r-type-client PRIVATE Threads::Threads)

# Configure ASIO
if(ASIO_INCLUDE_DIR)
    target_include_directories(r-type-client PRIVATE ${ASIO_INCLUDE_DIR})
    target_compile_definitions(r-type-client PRIVATE ASIO_STANDALONE)
else()
    target_include_directories(r-type-client PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(r-type-client PRIVATE ${Boost_LIBRARIES})
endif()

# Compiler settings
target_compile_features(r-type-client PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(r-type-client PRIVATE /W4 /permissive-)
else()
    target_compile_options(r-type-client PRIVATE -Wall -Wextra -Wpedantic)
endif()

# On Linux: copy to repo root for convenience
if(UNIX AND NOT WIN32)
    add_custom_command(TARGET r-type-client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:r-type-client>" "${CMAKE_SOURCE_DIR}/r-type-client"
        COMMAND chmod +x "${CMAKE_SOURCE_DIR}/r-type-client"
        COMMENT "Copying r-type-client executable to repository root"
    )
endif()
