# Building R-Type with CMake

## Prerequisites

- CMake 3.17 or higher
- C++17 compatible compiler (GCC, Clang, MSVC)
- ASIO (standalone) or Boost.Asio
- Raylib (local directory or system-installed)

### Installing Dependencies (Ubuntu/Debian)

```bash
sudo apt-get install cmake g++ libasio-dev
```

## Build Instructions

### 1. Configure the project

```bash
mkdir build
cd build
cmake ..
```

### 2. Build all targets

```bash
cmake --build . -j$(nproc)
```

Or using make:

```bash
make -j$(nproc)
```

### 3. Run the executables

All executables are in the `build/` directory:

```bash
# Server
./r-type_server <tcp_port> <udp_port>
# Example: ./r-type_server 4242 4243

# Render Client (Raylib graphical client)
./render_client <host> <tcp_port>
# Example: ./render_client localhost 4242

# Game Client (interactive console)
./game_client <host> <tcp_port>

# Test clients
./client_test <host> <tcp_port>
./test_headless <host> <tcp_port>
```

## Raylib Integration

The CMakeLists.txt automatically detects Raylib in two ways:

1. **Local directory** (recommended): If `raylib/` exists in the project root, it will be built automatically
2. **System-installed**: If Raylib is installed system-wide, it will be used

If Raylib is not found, only `render_client` will be skipped. Other targets will build normally.

## Build Options

### Release Build (optimized)

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

### Debug Build (with debug symbols)

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

## Installation

To install executables to `/usr/local/bin` (or custom prefix):

```bash
sudo cmake --install .
```

Or with custom prefix:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/rtype ..
cmake --build .
cmake --install .
```

## Troubleshooting

### ASIO not found

```bash
# Install standalone ASIO
sudo apt-get install libasio-dev

# Or use Boost.Asio
sudo apt-get install libboost-all-dev
```

### Raylib not found

Clone raylib into the project:

```bash
cd /path/to/rtype
git clone https://github.com/raysan5/raylib.git
```

Or install system-wide:

```bash
sudo apt-get install libraylib-dev  # Ubuntu 22.04+
```

### Clean build

```bash
rm -rf build/
mkdir build
cd build
cmake ..
cmake --build .
```

## Comparison with Makefile

Both build systems are supported:

| Feature | Makefile | CMake |
|---------|----------|-------|
| Build all | `make` | `cmake --build build` |
| Clean | `make fclean` | `rm -rf build` |
| Parallel build | `make -j$(nproc)` | `cmake --build build -j$(nproc)` |
| Cross-platform | Linux/macOS | Linux/macOS/Windows |
| IDE support | Limited | Excellent (CLion, VSCode, Visual Studio) |

## IDE Integration

### CLion

Open the project directory in CLion. It will automatically detect the CMakeLists.txt and configure the project.

### VSCode

Install the CMake Tools extension and open the project. Press `Ctrl+Shift+P` and select "CMake: Configure".

### Visual Studio (Windows)

Use "Open Folder" and select the project root. Visual Studio will detect CMakeLists.txt automatically.
