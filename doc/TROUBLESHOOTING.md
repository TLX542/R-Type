# R-Type Troubleshooting Guide

This guide helps diagnose and fix common issues with the R-Type server and client.

## Black Screen Issue

### Symptoms
- Client window opens but shows only a black screen
- No entities are visible
- Server shows "player connected" but no rendering happens

### Root Cause (Fixed)
The issue was caused by a timing problem between TCP connection and UDP initialization:
1. Client connects via TCP and gets authenticated
2. Server immediately sends ENTITY_SPAWN packets
3. BUT: Client hasn't sent its first UDP packet yet
4. Result: Server's UDP endpoint for the client is not initialized (`udpInitialized = false`)
5. ENTITY_SPAWN packets are not sent because UDP is not ready

### Solution
The fix implements a two-stage connection process:
1. **TCP Connection**: Player connects and entity is created in ECS
2. **UDP Ready**: When first UDP packet is received, ENTITY_SPAWN is broadcast

The `onPlayerUdpReady()` callback ensures packets are only sent after the client's UDP endpoint is properly initialized.

## Debugging Steps

### 1. Enable Verbose Logging

In `GameClient.cpp`, set `VERBOSE_LOGGING = true`:
```cpp
void GameClient::update() {
    // Set to true for verbose packet logging
    static const bool VERBOSE_LOGGING = true;  // Change this
    ...
}
```

In `render_client.cpp`, set `VERBOSE_LOGGING = true`:
```cpp
    static const bool VERBOSE_LOGGING = true;  // Change this
```

### 2. Use the Headless Test Client

Run the automated test client to verify connectivity without graphics:
```bash
./test_headless localhost 4242
```

**Expected output:**
```
[Client] Connecting to localhost:4242...
[Client] TCP connected
[Client] Authenticated!
         Player ID: 1
         Token: 0x...
         UDP Port: 4243
[Client] UDP socket configured for 127.0.0.1:4243
[Client] Sending initial UDP packet...
[Client] Ready to play!
[Test] Second 0: 1 entities
  Entity 1: pos(100, 300) color(200,30,30)
...
SUCCESS: Entities received and rendered!
```

**If you see "Final entity count: 0":**
- Check if server is running on the correct ports
- Verify firewall is not blocking UDP port 4243
- Check server logs for errors

### 3. Check Server Logs

**Expected server logs:**
```
R-Type Server started
TCP port: 4242
UDP port: 4243
[GameServer] ECS initialized
[GameServer] Game loop started
[TCP] Client #1 connected from 127.0.0.1
[TCP] Client #1 (TestPlayer) authenticated
[GameServer] Player 1 connected (TCP)
[GameServer] Creating entity for player 1 (will spawn when UDP ready)
[UDP] Received packet from 127.0.0.1:xxxxx (17 bytes)
[UDP] Client #1 endpoint initialized: 127.0.0.1:xxxxx
[GameServer] Player 1 UDP ready, sending ENTITY_SPAWN
[GameServer] Broadcasting ENTITY_SPAWN for network ID 1 to 1 sessions
[GameServer] Sent ENTITY_SPAWN to client #1 at 127.0.0.1:xxxxx
```

**Key checkpoints:**
- ✅ `Player 1 connected (TCP)` - TCP connection successful
- ✅ `Creating entity for player 1` - Entity created in ECS
- ✅ `Client #1 endpoint initialized` - UDP connection ready
- ✅ `Player 1 UDP ready, sending ENTITY_SPAWN` - Spawn packet sent
- ✅ `Broadcast update X: 1 entities to 1 clients` - Position updates sent

**If UDP endpoint is never initialized:**
- Client may not be sending UDP packets
- Firewall may be blocking UDP
- Check client logs for UDP send errors

## Common Issues

### Issue: Windows Build - Raylib Symbol Collisions

**Symptoms:**
- Build fails on Windows with MSVC due to symbol redefinition errors
- Errors mention `Rectangle`, `CloseWindow`, `ShowCursor`, `DrawText`, etc.
- Conflicts between raylib and Windows SDK (GDI) declarations

**Solution:**
The render_client target is configured with `WIN32_LEAN_AND_MEAN`, `NOMINMAX`, and `NOGDI` compile definitions by default on Windows builds. This prevents name collisions with the Windows SDK. No manual changes required - the CMake configuration handles this automatically.

### Issue: "No entities received"

**Diagnosis:**
```bash
# Check if server is running
ps aux | grep r-type_server

# Check if ports are in use
netstat -tuln | grep 4242
netstat -tuln | grep 4243
```

**Solutions:**
1. Ensure server is running: `./r-type_server 4242 4243`
2. Check firewall: `sudo ufw allow 4242/tcp && sudo ufw allow 4243/udp`
3. Verify network connectivity: `ping localhost`

### Issue: "Connection refused"

**Causes:**
- Server not running
- Wrong port numbers
- Firewall blocking connections

**Solutions:**
1. Start server first: `./r-type_server 4242 4243`
2. Verify port numbers match between server and client
3. Check firewall settings

### Issue: "Entities not moving"

**Causes:**
- Game loop not running
- Input not being processed
- Network packets not reaching server

**Solutions:**
1. Check server logs for "Broadcast update" messages
2. Send input and verify server logs show "Player X → Direction: [...]"
3. Enable verbose logging to see packet processing

### Issue: "Multiple clients not syncing"

**Causes:**
- Each client only sees their own entity
- ENTITY_SPAWN not broadcast to all clients

**Check:**
1. Server should log "Broadcasting ENTITY_SPAWN ... to N sessions"
2. Each client should receive spawn packets for all players
3. Enable verbose logging on clients to verify packet reception

## Performance Issues

### High CPU Usage

**Server side:**
- Game loop running at 60 Hz is normal
- High CPU may indicate too many entities
- Check entity count in logs

**Client side:**
- SFML rendering at 60 FPS is normal
- Ensure window frame rate limit is set: `window.setFramerateLimit(60)`

### High Network Usage

**Normal bandwidth:**
- ~40 KB/s per client at 60 Hz update rate
- ENTITY_BATCH_UPDATE packets sent every frame

**Reduce bandwidth:**
- Decrease tick rate (modify `TICK_RATE` in GameServer.hpp)
- Send updates less frequently
- Use delta compression (not implemented yet)

## Logging Reference

### Client Logs

**Connection phase:**
- `[Client] Connecting to...` - Starting TCP connection
- `[Client] TCP connected` - TCP connection established
- `[Client] Authenticated!` - Received CONNECT_OK from server
- `[Client] UDP socket configured` - UDP ready to send
- `[Client] Ready to play!` - Fully connected

**Runtime:**
- `[Client] Entity spawned: ID=X` - New entity received
- `[Client] Entity destroyed: ID=X` - Entity removed
- `[Client] Received update for unknown entity` - Received update before spawn (ERROR)

### Server Logs

**Connection phase:**
- `[TCP] Client #X connected from ...` - TCP connection accepted
- `[TCP] Client #X (...) authenticated` - Client authenticated
- `[GameServer] Player X connected (TCP)` - Player registered
- `[UDP] Client #X endpoint initialized` - First UDP packet received
- `[GameServer] Player X UDP ready` - About to send ENTITY_SPAWN

**Runtime:**
- `[UDP] Received packet from ...` - UDP packet received
- `[UDP] Player X → Direction: [...]` - Input processed
- `[GameServer] Broadcast update X` - Position updates sent (every 60 frames)
- `[TCP] Client #X disconnected` - Player left

## Testing Checklist

Before reporting a bug, please verify:

- [ ] Server starts without errors
- [ ] Server shows "Game loop started"
- [ ] Client connects successfully (TCP)
- [ ] Client sends first UDP packet
- [ ] Server shows "UDP endpoint initialized"
- [ ] Server sends ENTITY_SPAWN
- [ ] Client receives and processes ENTITY_SPAWN
- [ ] Client receives ENTITY_BATCH_UPDATE packets
- [ ] Entities appear on screen

Use the headless test client to verify each step:
```bash
# Terminal 1
./r-type_server 4242 4243

# Terminal 2
./test_headless localhost 4242
```

## Getting Help

If issues persist after following this guide:

1. **Enable verbose logging** and capture full output
2. **Run the headless test client** and note the results
3. **Collect server logs** from the connection attempt
4. **Check system requirements:**
   - C++17 compiler
   - ASIO standalone or Boost.Asio
   - SFML 2.5+ (for render_client)
   - Linux/Mac/Windows with network access

5. **Include in your report:**
   - Operating system and version
   - Server command used
   - Client command used
   - Server logs (full output)
   - Client logs (full output)
   - Error messages
   - Expected vs actual behavior

## Advanced Debugging

### Packet Capture

Use tcpdump to inspect network traffic:
```bash
# Capture UDP packets on port 4243
sudo tcpdump -i lo -nn port 4243 -X

# Capture TCP packets on port 4242
sudo tcpdump -i lo -nn port 4242 -X
```

### GDB Debugging

Debug the server:
```bash
gdb ./r-type_server
(gdb) run 4242 4243
(gdb) bt  # If it crashes, show backtrace
```

Debug the client:
```bash
gdb ./render_client
(gdb) run localhost 4242
```

### Valgrind Memory Check

Check for memory leaks:
```bash
valgrind --leak-check=full ./r-type_server 4242 4243
valgrind --leak-check=full ./test_headless localhost 4242
```
