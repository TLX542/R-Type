# CMake for building the executable from src/ and include/
cmake_minimum_required(VERSION 3.15)

# Collect all source files in src/
file(GLOB_RECURSE PROJECT_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

if (PROJECT_SOURCES STREQUAL "")
    message(FATAL_ERROR "No source files found in src/ (expected at least src/main.cpp)")
endif()

add_executable(bs-rtype ${PROJECT_SOURCES})

# Include public headers
target_include_directories(bs-rtype PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Compiler warnings / flags
if (MSVC)
    target_compile_options(bs-rtype PRIVATE /W4 /permissive-)
    # Disable some noisy warnings if needed:
    # target_compile_definitions(bs-rtype PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(bs-rtype PRIVATE -Wall -Wextra -Wpedantic)
    # You can add -Werror if you want to be strict
endif()

# Find SFML (requires SFML 2.5+)
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

if (SFML_FOUND)
    target_link_libraries(bs-rtype PRIVATE sfml-graphics sfml-window sfml-system)
else()
    message(FATAL_ERROR "SFML not found. Install SFML development packages or set SFML_DIR.")
endif()

# When linking SFML statically on Windows you may need to define SFML_STATIC:
if (WIN32 AND NOT BUILD_SHARED_SFML)
    target_compile_definitions(bs-rtype PRIVATE SFML_STATIC)
    # NOTE: When linking SFML statically you might also need to link SFML's dependencies
    # such as opengl32, freetype, winmm, gdi32, ws2_32, etc. How to do that depends on
    # the SFML package you used (vcpkg/MSYS2/official SDK).
    #
    # Example (uncomment if you know you need them):
    # target_link_libraries(bs-rtype PRIVATE opengl32 freetype winmm gdi32 ws2_32)
endif()