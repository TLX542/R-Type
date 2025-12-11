cmake_minimum_required(VERSION 3.15)
project(R-Type VERSION 1.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to choose whether to link SFML dynamically (default ON)
option(BUILD_SHARED_SFML "Link SFML dynamically (ON) or statically (OFF)" ON)

# Add subdirectories
# 1. Game library (shared game logic)
add_subdirectory(src/game)

# 2. Server executable
add_subdirectory(Serveur)

# 3. Client executable
add_subdirectory(src/client)

# 4. Original src directory (for backwards compatibility with bs-rtype)
add_subdirectory(src)

# Install targets
install(TARGETS bs-rtype r-type-server r-type-client
  RUNTIME DESTINATION bin
)