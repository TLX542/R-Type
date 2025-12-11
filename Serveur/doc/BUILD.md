# Building and Running R-Type Server and Client

## Prerequisites

### All Platforms
- **C++17 compiler** (g++, clang, or MSVC)
- **Make** (for building with Makefile)
- **CMake** >= 3.15 (optional, for alternative build)

### Linux (Ubuntu/Debian)
```bash
sudo apt-get update
sudo apt-get install -y build-essential g++ make cmake
sudo apt-get install -y libasio-dev      # For network (ASIO standalone)
sudo apt-get install -y libsfml-dev      # For client rendering
```

### macOS
```bash
brew install cmake
brew install asio
brew install sfml
```

### Windows
- Install Visual Studio 2019+ or MinGW-w64
- Install dependencies via vcpkg:
  ```cmd
  vcpkg install asio:x64-windows
  vcpkg install sfml:x64-windows
  ```

## Building

### Server Only

Navigate to the `Serveur` directory:

```bash
cd Serveur
```

**Build the server:**
```bash
make r-type_server
```

This creates the `r-type_server` executable.

**Clean build:**
```bash
make clean      # Remove object files
make fclean     # Remove object files and binaries
make re         # Clean and rebuild
```

### Client Only

**Build the render client (requires SFML):**
```bash
cd Serveur
make render_client
```

This creates the `render_client` executable.

### All Targets

**Build everything:**
```bash
cd Serveur
make
```

This builds:
- `r-type_server` - Game server with ECS
- `render_client` - SFML-based rendering client
- `game_client` - Terminal-based interactive client
- `client_test` - Simple echo test client
- `protocol_test` - Protocol testing client

## Running

### 1. Start the Server

The server requires two ports: one for TCP (connection) and one for UDP (gameplay).

```bash
cd Serveur
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
cd Serveur
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
cd Serveur
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
cd Serveur
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
cd Serveur
CXXFLAGS="-std=c++17 -g -O0" make re
```

For release with optimizations:

```bash
cd Serveur
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
