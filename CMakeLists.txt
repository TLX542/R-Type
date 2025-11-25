cmake_minimum_required(VERSION 3.15)
project(bs-rtype LANGUAGES CXX)

# Set a sensible default
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Allow the user to point to a non-standard SFML location
# Example: -DCMAKE_PREFIX_PATH=/mingw64
if(NOT DEFINED CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "" CACHE STRING "Additional prefix path for find_package")
endif()

# Add src directory (this defines the bs-rtype target and the dist target on Windows)
add_subdirectory(src)

# Optionally add packaging/ZIP target if desired (left out for clarity)