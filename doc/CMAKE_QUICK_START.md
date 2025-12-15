# CMake Quick Start Guide

Quick reference for building R-Type with CMake on any platform.

## TL;DR

```bash
# Linux/macOS
mkdir build && cd build
cmake ..
cmake --build . -j8
./r-type_server 4242 4243

# Windows (PowerShell)
mkdir build; cd build
cmake ..
cmake --build . --config Release
.\Release\r-type_server.exe 4242 4243
```

## One-Liner Builds

### Linux/macOS
```bash
mkdir -p build && cd build && cmake .. && cmake --build . -j8 && cd ..
```

### Windows
```powershell
if (!(Test-Path build)) { mkdir build }; cd build; cmake ..; cmake --build . --config Release; cd ..
```

## Common CMake Commands

### Configure
```bash
cmake ..                                    # Default configuration
cmake -DCMAKE_BUILD_TYPE=Release ..         # Release build
cmake -DCMAKE_BUILD_TYPE=Debug ..           # Debug build
cmake -DUSE_SYSTEM_DEPENDENCIES=ON ..       # Use system deps only
cmake -DBUILD_TESTS=OFF ..                  # Skip test executables
```

### Build
```bash
cmake --build .                             # Build all targets
cmake --build . -j8                         # Parallel build (8 jobs)
cmake --build . --config Release            # Windows: Release build
cmake --build . --target r-type_server      # Build specific target
cmake --build . --verbose                   # Show full commands
```

### Clean
```bash
cmake --build . --target clean              # Clean build artifacts
rm -rf build                                # Start completely fresh
```

## Build Targets

Available targets:
- `r-type_server` - Game server
- `client_test` - Simple TCP test client
- `game_client` - Interactive console client
- `render_client` - Graphical client (raylib) *
- `test_headless` - Automated test client **

\* Only built if raylib is available
** Only built if BUILD_TESTS=ON (default)

## CMake Options Reference

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | - | Debug, Release, RelWithDebInfo, MinSizeRel |
| `USE_SYSTEM_DEPENDENCIES` | OFF | Force using system packages |
| `USE_VCPKG` | OFF | Enable vcpkg hints |
| `BUILD_TESTS` | ON | Build test executables |
| `BUILD_EXAMPLES` | OFF | Build examples (future) |

## Platform-Specific Quick Start

### Ubuntu/Debian
```bash
# Option 1: Let CMake download dependencies
mkdir build && cd build && cmake .. && cmake --build . -j8

# Option 2: Use system packages
sudo apt-get install libasio-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
mkdir build && cd build
cmake -DUSE_SYSTEM_DEPENDENCIES=ON ..
cmake --build . -j8
```

### Arch Linux
```bash
# Option 1: Let CMake download
mkdir build && cd build && cmake .. && cmake --build . -j8

# Option 2: Use system packages
sudo pacman -S asio
mkdir build && cd build
cmake -DUSE_SYSTEM_DEPENDENCIES=ON ..
cmake --build . -j8
```

### macOS
```bash
# Option 1: Let CMake download
mkdir build && cd build && cmake .. && cmake --build . -j8

# Option 2: Use Homebrew
brew install asio raylib
mkdir build && cd build
cmake -DUSE_SYSTEM_DEPENDENCIES=ON ..
cmake --build . -j8
```

### Windows (Visual Studio)
```powershell
# Automatic dependencies (recommended)
mkdir build; cd build
cmake ..
cmake --build . --config Release

# With vcpkg
.\scripts\bootstrap-deps.ps1
mkdir build; cd build
cmake -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake ..
cmake --build . --config Release
```

### Windows (MinGW)
```bash
# In Git Bash or MSYS2
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
cmake --build . -j8
```

## Troubleshooting Quick Fixes

### "ASIO not found"
```bash
# Let CMake download it
cmake -DUSE_SYSTEM_DEPENDENCIES=OFF ..

# Or install manually
# Ubuntu: sudo apt-get install libasio-dev
# Arch:   sudo pacman -S asio
# macOS:  brew install asio
```

### "raylib not found" (Linux)
```bash
# Install X11 libraries
sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev

# Let CMake download raylib
cmake -DUSE_SYSTEM_DEPENDENCIES=OFF ..
```

### "Git not found"
Install Git:
- Ubuntu: `sudo apt-get install git`
- Windows: Download from https://git-scm.com/download/win
- macOS: `brew install git`

### CMake version too old
```bash
# Ubuntu (add Kitware APT repository)
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update
sudo apt-get install cmake

# macOS
brew upgrade cmake

# Windows
# Download latest from https://cmake.org/download/
```

### Build cache issues
```bash
# Clean and reconfigure
rm -rf build
mkdir build && cd build
cmake ..
```

## Running Quick Tests

### Start Server
```bash
# Linux/macOS
cd build
./r-type_server 4242 4243

# Windows
cd build\Release
.\r-type_server.exe 4242 4243
```

### Start Client
```bash
# Linux/macOS
cd build
./render_client localhost 4242

# Windows
cd build\Release
.\render_client.exe localhost 4242
```

### Run Automated Test
```bash
# Linux/macOS
cd build
./test_headless localhost 4242

# Windows
cd build\Release
.\test_headless.exe localhost 4242
```

## IDE Integration

### Visual Studio Code
1. Install CMake Tools extension
2. Open project folder
3. Press `Ctrl+Shift+P` → "CMake: Configure"
4. Press `F7` to build

### CLion
1. Open project directory
2. CLion auto-detects CMakeLists.txt
3. Click build button or press `Ctrl+F9`

### Visual Studio 2019/2022
1. File → Open → Folder
2. Select project directory
3. VS detects CMakeLists.txt automatically
4. Press `Ctrl+Shift+B` to build

## Environment Variables

Useful environment variables for CMake:

```bash
# Specify toolchain file
export CMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Set install prefix
export CMAKE_INSTALL_PREFIX=/usr/local

# Specify build type
export CMAKE_BUILD_TYPE=Release

# Set generator
export CMAKE_GENERATOR="Unix Makefiles"
# or "Ninja", "Visual Studio 17 2022", etc.
```

## More Information

- Full build guide: [BUILD.md](BUILD.md)
- Windows guide: [WINDOWS_CMAKE_GUIDE.md](WINDOWS_CMAKE_GUIDE.md)
- Architecture: [ARCHITECTURE.md](ARCHITECTURE.md)
- Troubleshooting: [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
