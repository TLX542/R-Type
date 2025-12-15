# Building and Running R-Type

This guide covers building R-Type on Linux, Windows, and macOS using the modern CMake build system with **automatic dependency management**.

## 🚀 What's New

The R-Type build system now features:
- **Automatic Dependency Download**: CMake automatically fetches ASIO and raylib if not found on your system
- **Cross-Platform Support**: Builds on Linux, Windows (MSVC/MinGW), and macOS
- **Multiple Build Methods**: System packages, vcpkg, or automatic FetchContent
- **CMake GUI Support**: Easy configuration on Windows
- **Modern CMake**: Target-based linking with proper dependency management
- **Bootstrap Scripts**: Helper scripts for easy dependency installation

## Table of Contents
- [Quick Start](#quick-start)
- [Prerequisites](#prerequisites)
- [Building on Linux](#building-on-linux)
- [Building on Windows](#building-on-windows)
- [Building on macOS](#building-on-macos)
- [Build Options](#build-options)
- [Running](#running)
- [Troubleshooting](#troubleshooting)

## Quick Start

The easiest way to build R-Type is to let CMake automatically download and build dependencies:

**Linux/macOS:**
```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

**Windows (PowerShell):**
```powershell
mkdir build; cd build
cmake ..
cmake --build . --config Release
```

**That's it!** CMake will automatically download ASIO and raylib if they're not installed on your system.

**Note**: On Linux, you may need X11 development libraries for raylib:
```bash
sudo apt-get install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

## Prerequisites

### All Platforms
- **C++17 compatible compiler**
  - Linux: GCC 7+ or Clang 5+
  - Windows: Visual Studio 2019+ or MinGW-w64
  - macOS: Xcode 10+ (Clang)
- **CMake** >= 3.16
- **Git** (for FetchContent to download dependencies)

### Optional: System Package Manager
If you prefer using system-installed dependencies instead of auto-download, see platform-specific sections below.

## Building on Linux

### Option 1: Automatic Dependencies (Recommended)

This is the simplest method - CMake downloads and builds dependencies automatically:

```bash
# Clone the repository (if not already done)
git clone https://github.com/TLX542/R-Type.git
cd R-Type

# Create build directory
mkdir build
cd build

# Configure (this downloads dependencies automatically)
cmake ..

# Build with parallel jobs
cmake --build . -j$(nproc)

# Executables are in the build directory
ls -l r-type_server render_client game_client
```

### Option 2: Using System Dependencies

First, run the bootstrap script to install system dependencies:

```bash
# Install system dependencies
./scripts/bootstrap-deps.sh

# Build with system dependencies
mkdir build && cd build
cmake -DUSE_SYSTEM_DEPENDENCIES=ON ..
cmake --build . -j$(nproc)
```

The bootstrap script supports multiple package managers:
- **Ubuntu/Debian**: Uses `apt-get`
- **Arch Linux**: Uses `pacman`
- **Fedora/RHEL**: Uses `dnf`
- **macOS**: Uses `brew`

### Option 3: Using vcpkg

```bash
# Install dependencies via vcpkg
./scripts/bootstrap-deps.sh --vcpkg

# Build with vcpkg toolchain
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ..
cmake --build . -j$(nproc)
```

## Building on Windows

### Option 1: Using CMake GUI (Easiest for Windows)

1. **Install Prerequisites**
   - Download and install [CMake](https://cmake.org/download/) (3.16 or later)
   - Download and install [Visual Studio 2019/2022](https://visualstudio.microsoft.com/) with "Desktop development with C++"
   - Download and install [Git for Windows](https://git-scm.com/download/win)

2. **Run Bootstrap Script** (Optional but recommended)
   ```powershell
   # In PowerShell, navigate to project directory
   cd R-Type
   .\scripts\bootstrap-deps.ps1
   ```
   This installs dependencies via vcpkg and shows build instructions.

3. **Build with CMake GUI**
   - Open CMake GUI
   - Set "Where is the source code" to: `C:\path\to\R-Type`
   - Set "Where to build the binaries" to: `C:\path\to\R-Type\build`
   - Click **Configure**
   - Select your generator (e.g., "Visual Studio 17 2022")
   - Click **Generate**
   - Click **Open Project** to open in Visual Studio
   - In Visual Studio: Build > Build Solution (or press F7)

4. **Executables** will be in `build\Release\` or `build\Debug\`

### Option 2: Command Line with vcpkg

```powershell
# Run bootstrap script to setup vcpkg
.\scripts\bootstrap-deps.ps1

# Create build directory
mkdir build
cd build

# Configure with vcpkg toolchain
cmake -DCMAKE_TOOLCHAIN_FILE=..\vcpkg\scripts\buildsystems\vcpkg.cmake ..

# Build
cmake --build . --config Release

# Executables are in build\Release\
```

### Option 3: Automatic Dependencies (No vcpkg)

```powershell
# Create build directory
mkdir build
cd build

# Configure (downloads dependencies automatically)
cmake ..

# Build
cmake --build . --config Release
```

### Option 4: Using Visual Studio 2019/2022 Directly

1. Open Visual Studio
2. File → Open → Folder
3. Select the R-Type project directory
4. Visual Studio detects CMakeLists.txt automatically
5. If using vcpkg, edit `CMakeSettings.json` to add:
   ```json
   "cmakeCommandArgs": "-DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
   ```
6. Build → Build All (Ctrl+Shift+B)

## Building on macOS

### Option 1: Automatic Dependencies

```bash
# Install Xcode Command Line Tools (if not already installed)
xcode-select --install

# Install CMake via Homebrew
brew install cmake

# Build
mkdir build && cd build
cmake ..
cmake --build . -j$(sysctl -n hw.ncpu)
```

### Option 2: Using Homebrew Dependencies

```bash
# Install dependencies
brew install cmake asio raylib

# Build
mkdir build && cd build
cmake -DUSE_SYSTEM_DEPENDENCIES=ON ..
cmake --build . -j$(sysctl -n hw.ncpu)
```

## Build Options

CMake provides several options to customize the build:

| Option | Default | Description |
|--------|---------|-------------|
| `USE_SYSTEM_DEPENDENCIES` | OFF | Force using system-installed dependencies only |
| `USE_VCPKG` | OFF | Enable vcpkg integration hints and messages |
| `BUILD_EXAMPLES` | OFF | Build example executables (future use) |
| `BUILD_TESTS` | ON | Build test executables (test_headless) |
| `CMAKE_BUILD_TYPE` | - | Build type: Debug, Release, RelWithDebInfo, MinSizeRel |

### Examples

```bash
# Force using system dependencies
cmake -DUSE_SYSTEM_DEPENDENCIES=ON ..

# Disable building tests
cmake -DBUILD_TESTS=OFF ..

# Enable vcpkg hints
cmake -DUSE_VCPKG=ON ..

# Debug build with symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release ..

# Combine multiple options
cmake -DUSE_SYSTEM_DEPENDENCIES=ON -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release ..
```

## Running

### 1. Start the Server

The server requires two ports: one for TCP (connection) and one for UDP (gameplay).

```bash
./r-type_server 4242 4243
```

Expected output:
```
R-Type Server started
TCP port: 4242
UDP port: 4243
[GameServer] ECS initialized
[GameServer] Game loop started
R-Type Game Server is running...
Waiting for clients to connect...
```

The server will:
- Accept TCP connections on port 4242
- Listen for UDP packets on port 4243
- Run the game loop at 60 ticks/second
- Wait for clients to connect

### 2. Start Client(s)

**Option A: SFML Rendering Client (Recommended)**

In a new terminal:
```bash
./render_client localhost 4242
```

You'll be prompted for a username:
```
Enter your name: Player1
```

A window will open showing the game. You can control your square with:
- **Arrow Keys**: Move up/down/left/right
- **Space**: Shoot (not implemented yet)
- **ESC**: Quit

**Option B: Terminal Interactive Client**

```bash
./game_client localhost 4242
```

This provides a terminal-based interface with manual input controls.

### 3. Multiple Players

To test multiplayer, open multiple terminals and start multiple clients:

**Terminal 1:**
```bash
./render_client localhost 4242
# Enter name: Player1
```

**Terminal 2:**
```bash
./render_client localhost 4242
# Enter name: Player2
```

**Terminal 3:**
```bash
./render_client localhost 4242
# Enter name: Player3
```

Each player will control a square with a different color:
- Player 1: Red
- Player 2: Green
- Player 3: Blue
- Player 4: Yellow

All players see the same game world in real-time!

## Quick Demo Script

For a quick test, you can use this script:

```bash
#!/bin/bash

# Terminal 1: Start server
./r-type_server 4242 4243 &
SERVER_PID=$!

# Wait for server to start
sleep 1

# Terminal 2: Start client 1
./render_client localhost 4242 &
CLIENT1_PID=$!

# Terminal 3: Start client 2
./render_client localhost 4242 &
CLIENT2_PID=$!

echo "Press Enter to stop..."
read

# Cleanup
kill $CLIENT1_PID $CLIENT2_PID $SERVER_PID
```

## Troubleshooting

### "Address already in use"
Another process is using the port. Find and kill it:
```bash
# Find process using port 4242
lsof -i :4242
# or
netstat -tuln | grep 4242

# Kill the process
kill -9 <PID>
```

### "asio.hpp: No such file or directory"
ASIO is not installed. Install it:
```bash
# Linux
sudo apt-get install libasio-dev

# macOS
brew install asio
```

### "SFML/Graphics.hpp: No such file or directory"
SFML is not installed (needed for render_client):
```bash
# Linux
sudo apt-get install libsfml-dev

# macOS
brew install sfml
```

### Server starts but client can't connect
1. Check firewall rules
2. Try using `127.0.0.1` instead of `localhost`
3. Verify server is listening:
   ```bash
   netstat -tuln | grep 4242
   ```

### Client connects but can't move
1. Check server logs - should show "Player X input" messages
2. Verify UDP port 4243 is not blocked
3. Make sure game loop started (check server output)

### Black screen on client
1. Window opened but no entities yet
2. Wait for server to spawn player entity
3. Check server logs for "Entity spawned" messages

### Choppy movement
1. Increase server tick rate (edit `GameServer::TICK_RATE`)
2. Reduce network latency (use local network)
3. Check CPU usage (game loop might be too slow)

## Development Build

For development with debug symbols:

```bash
CXXFLAGS="-std=c++17 -g -O0" make re
```

For release with optimizations:

```bash
CXXFLAGS="-std=c++17 -O3 -DNDEBUG" make re
```

## Testing Network Protocol

To test just the network protocol without rendering:

```bash
# Terminal 1: Server
./r-type_server 4242 4243

# Terminal 2: Protocol test client
./protocol_test localhost 4242
```

This sends test packets and displays responses.

## Platform-Specific Notes

### Linux
- Default compiler: `g++`
- ASIO: Standalone version preferred
- SFML: Install from package manager

### macOS
- May need to use `clang++`
- SFML: Install via Homebrew
- Use `lldb` for debugging instead of `gdb`

### Windows
- Use MinGW or Visual Studio
- ASIO: Use vcpkg or Boost.Asio
- SFML: Use vcpkg or download binaries
- May need to add DLLs to executable directory

## Next Steps

After successfully building and running:
1. Read [ARCHITECTURE.md](ARCHITECTURE.md) to understand the design
2. Read [PROTOCOL.md](PROTOCOL.md) for network protocol details
3. Try modifying game parameters (player speed, colors, etc.)
4. Implement new features (shooting, enemies, etc.)

## Getting Help

If you encounter issues:
1. Check server logs for error messages
2. Verify all dependencies are installed
3. Test with simple clients first (`client_test`)
4. Check protocol communication with `protocol_test`
5. Review the [ARCHITECTURE.md](ARCHITECTURE.md) documentation
