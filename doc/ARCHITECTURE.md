# R-Type Server-Client Architecture

## Overview

The R-Type project uses a **server-authoritative architecture** where:
- The **server** runs the complete game simulation using an ECS (Entity-Component-System)
- **Clients** are thin renderers that only send input and display the world state received from the server

This architecture ensures:
- No cheating (server has full authority)
- Consistent game state across all clients
- Simplified client logic (just input + rendering)

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                        SERVER                                │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  GameServer (Authoritative)                            │ │
│  │  ┌──────────────┐    ┌──────────────┐                 │ │
│  │  │   Network    │    │  Game Loop   │                 │ │
│  │  │   (TCP/UDP)  │    │  (60 Hz)     │                 │ │
│  │  └──────┬───────┘    └───────┬──────┘                 │ │
│  │         │                    │                         │ │
│  │         │    ┌───────────────▼────────────────┐       │ │
│  │         │    │        ECS Registry             │       │ │
│  │         │    │  ┌────────────────────────┐    │       │ │
│  │         │    │  │ Components:            │    │       │ │
│  │         │    │  │  - Position            │    │       │ │
│  │         │    │  │  - Velocity            │    │       │ │
│  │         │    │  │  - Drawable            │    │       │ │
│  │         │    │  │  - NetworkId           │    │       │ │
│  │         │    │  │  - PlayerOwner         │    │       │ │
│  │         │    │  └────────────────────────┘    │       │ │
│  │         │    └────────────────────────────────┘       │ │
│  │         │                                              │ │
│  └─────────┼──────────────────────────────────────────────┘ │
│            │                                                 │
└────────────┼─────────────────────────────────────────────────┘
             │
             │ UDP: World State (ENTITY_BATCH_UPDATE)
             │ UDP: Player Input (PLAYER_INPUT)
             │ TCP: Connection/Authentication
             │
      ┌──────┴──────┬──────────────┬──────────────┐
      │             │              │              │
┌─────▼─────┐ ┌────▼──────┐ ┌────▼──────┐ ┌────▼──────┐
│  Client 1 │ │ Client 2  │ │ Client 3  │ │ Client 4  │
│           │ │           │ │           │ │           │
│  ┌─────┐  │ │  ┌─────┐  │ │  ┌─────┐  │ │  ┌─────┐  │
│  │Input│  │ │  │Input│  │ │  │Input│  │ │  │Input│  │
│  └──┬──┘  │ │  └──┬──┘  │ │  └──┬──┘  │ │  └──┬──┘  │
│     │     │ │     │     │ │     │     │ │     │     │
│  ┌──▼───┐ │ │  ┌──▼───┐ │ │  ┌──▼───┐ │ │  ┌──▼───┐ │
│  │Render│ │ │  │Render│ │ │  │Render│ │ │  │Render│ │
│  └──────┘ │ │  └──────┘ │ │  └──────┘ │ │  └──────┘ │
└───────────┘ └───────────┘ └───────────┘ └───────────┘
```

## Components

### Server (`GameServer`)

**Responsibilities:**
- Accept TCP connections and authenticate players
- Spawn player entities in the ECS world
- Run game loop at fixed 60 Hz tick rate
- Process player inputs and update ECS components
- Run ECS systems (position, velocity, collision, etc.)
- Broadcast world state to all clients via UDP

**Key Classes:**
- `GameServer` - Main server class with ECS integration
- `registry` - ECS registry managing entities and components
- `Server` - Base network server (TCP/UDP)

**Game Loop:**
```cpp
while (running) {
    // 1. Process player inputs from UDP packets
    // 2. Update ECS systems (velocity → position)
    // 3. Apply game logic (collision, damage, etc.)
    // 4. Broadcast world state to all clients
    // 5. Sleep to maintain 60 Hz
}
```

### Client (`GameClient` + `render_client`)

**Responsibilities:**
- Connect to server via TCP and authenticate
- Send player input via UDP at high frequency
- Receive world state updates via UDP
- Store entity positions locally
- Render entities using SFML

**Key Classes:**
- `GameClient` - Network communication layer
- `render_client` - SFML rendering application

**Client Loop:**
```cpp
while (window.isOpen()) {
    // 1. Read keyboard input
    // 2. Send input to server via UDP
    // 3. Receive world state from server (non-blocking)
    // 4. Render all entities at their current positions
    // 5. Display frame
}
```

## ECS Integration

The server uses the existing ECS from `src/` with these components:

### Core Components

```cpp
struct Position {
    float x, y;  // World position
};

struct Velocity {
    float vx, vy;  // Velocity in pixels/second
};

struct Drawable {
    float width, height;
    Color color;  // Visual representation
};
```

### Network Components

```cpp
struct NetworkId {
    uint32_t id;  // Unique network identifier
};

struct PlayerOwner {
    uint8_t playerId;  // Which player owns this entity (1-4)
};
```

### ECS Flow

1. **Entity Creation**: When a player connects, server spawns an entity:
   ```cpp
   Entity player = registry.spawn_entity();
   registry.add_component<Position>(player, {100, 300});
   registry.add_component<Velocity>(player, {0, 0});
   registry.add_component<NetworkId>(player, {nextId++});
   registry.add_component<PlayerOwner>(player, {playerId});
   ```

2. **Input Processing**: Player input updates velocity:
   ```cpp
   void handlePlayerInput(playerId, moveX, moveY) {
       Entity player = playerEntities[playerId];
       auto& vel = registry.get_component<Velocity>(player);
       vel.vx = moveX * PLAYER_SPEED;
       vel.vy = moveY * PLAYER_SPEED;
   }
   ```

3. **Game Update**: Position system integrates velocity:
   ```cpp
   void updateGame(deltaTime) {
       for each entity with Position and Velocity {
           position.x += velocity.vx * deltaTime;
           position.y += velocity.vy * deltaTime;
       }
   }
   ```

4. **State Broadcast**: Server serializes entities to network packets:
   ```cpp
   void broadcastWorldState() {
       for each entity with Position and NetworkId {
           packet.add(networkId, position.x, position.y);
       }
       udp.broadcast(packet);
   }
   ```

## Network Protocol

### Connection Flow

1. **TCP Connection** (reliable, for setup):
   ```
   Client → Server: CONNECT username=Player1 version=1.0
   Server → Client: CONNECT_OK id=1 token=0x12345678 udp_port=4243
   ```

2. **UDP Setup** (unreliable, for gameplay):
   ```
   Client → Server: Initial PLAYER_INPUT packet (establishes UDP endpoint)
   Server: Stores client's UDP endpoint for future broadcasts
   ```

### Gameplay Packets (UDP)

**Client → Server: PLAYER_INPUT**
```cpp
struct PlayerInputPayload {
    uint32_t timestamp;   // Client timestamp (ms)
    uint8_t playerId;     // Player ID (1-4)
    uint8_t buttons;      // Button bitfield (SHOOT, SPECIAL)
    int8_t moveX;         // -1, 0, or 1
    int8_t moveY;         // -1, 0, or 1
};
```

**Server → Client: ENTITY_BATCH_UPDATE**
```cpp
struct EntityBatchUpdatePayload {
    uint8_t count;                          // Number of entities
    EntityBatchEntry entities[count];       // Array of entity states
};

struct EntityBatchEntry {
    uint32_t networkId;
    float posX, posY;
    uint8_t health;
};
```

**Server → Client: ENTITY_SPAWN**
```cpp
struct EntitySpawnPayload {
    uint32_t networkId;
    uint8_t entityType;     // PLAYER, ENEMY, BULLET, etc.
    uint8_t ownerPlayer;    // Which player owns this
    float posX, posY;
    float velocityX, velocityY;
    uint8_t health;
};
```

**Server → Client: ENTITY_DESTROY**
```cpp
struct EntityDestroyPayload {
    uint32_t networkId;     // ID of entity to remove
};
```

## Performance Characteristics

### Server
- **Tick Rate**: 60 Hz (16.67ms per tick)
- **Max Players**: 4 simultaneous
- **Network**: ~40 KB/s per client at 60 Hz (batch updates)
- **Thread Model**: Separate game loop thread + ASIO I/O thread

### Client
- **Frame Rate**: 60 FPS (SFML limit)
- **Input Rate**: Up to 60 inputs/second
- **Latency**: ~16-50ms typical (depends on network)

## Design Decisions

### Why Server-Authoritative?
- **Anti-cheat**: Clients cannot modify game state
- **Consistency**: All players see the same game state
- **Simplified clients**: No need to duplicate game logic

### Why UDP for Gameplay?
- **Low latency**: No retransmission delays
- **Acceptable packet loss**: Old positions are discarded anyway
- **Higher throughput**: Can send more frequent updates

### Why Batch Updates?
- **Reduced overhead**: One packet instead of N packets
- **Better utilization**: Fill UDP packet capacity
- **Simpler protocol**: Single message type for all entities

### Why 60 Hz Tick Rate?
- **Smooth gameplay**: Matches typical monitor refresh rate
- **Low latency**: Fast response to player input
- **Reasonable bandwidth**: Not too expensive for 4 players

## Extending the System

### Adding New Components
1. Define component in `Components.hpp`
2. Register in `GameServer` constructor
3. Create system to process it
4. Add to serialization if needed for clients

### Adding New Entity Types
1. Add to `EntityType` enum in `Protocol.hpp`
2. Spawn in appropriate system/event
3. Handle in client rendering

### Adding Game Logic
All game logic goes on the **server** in:
- `GameServer::updateGame()` - Physics, movement
- ECS systems - Collision, shooting, AI
- Event handlers - Spawn, destroy, score

Clients should **never** implement game logic - they only render!

## Troubleshooting

### Server won't start
- Check ports are not in use: `netstat -tuln | grep 4242`
- Ensure ASIO is installed: `sudo apt-get install libasio-dev`

### Client can't connect
- Verify server is running
- Check firewall rules
- Try `localhost` instead of hostname

### Entities don't move
- Check client is sending input (debug logs)
- Verify server game loop is running
- Check UDP packets are being received

### Laggy movement
- Reduce packet size (fewer entities per batch)
- Increase tick rate (higher CPU usage)
- Check network latency with `ping`

## Future Improvements

- **Prediction**: Client-side prediction for smoother movement
- **Interpolation**: Smooth entity positions between updates
- **Delta compression**: Only send changed values
- **Reliability layer**: Guarantee important events (spawn/destroy)
- **Snapshot history**: Support for lag compensation
