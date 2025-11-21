cmake_minimum_required(VERSION 3.15)
project(R-Type VERSION 1.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to choose whether to link SFML dynamically (default ON)
option(BUILD_SHARED_SFML "Link SFML dynamically (ON) or statically (OFF)" ON)

# If you want a multi-config generator, CMake will honor --config at build time.
# Add the src subdirectory which builds the bs-rtype executable.
add_subdirectory(src)

# Install target (optional)
install(TARGETS bs-rtype
  RUNTIME DESTINATION bin
)