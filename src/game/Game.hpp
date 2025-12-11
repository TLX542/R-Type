#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include "Registry.hpp"
#include "Components.hpp"

namespace game {

// Snapshot of an entity's state for network transmission
struct EntitySnapshot {
    uint32_t networkId;
    uint8_t entityType;  // 0=player, 1=enemy, 2=bullet, etc.
    float posX;
    float posY;
    float velX;
    float velY;
    float width;
    float height;
    uint8_t colorR;
    uint8_t colorG;
    uint8_t colorB;
    uint8_t health;
    uint8_t ownerPlayer;  // For player entities, which player (0-3)
};

// Input from a player
struct PlayerInput {
    uint8_t playerId;
    int8_t moveX;  // -1, 0, +1
    int8_t moveY;  // -1, 0, +1
    uint8_t buttons;  // Bitfield for shoot, special, etc.
    uint32_t timestamp;
};

// Full game state snapshot
struct GameSnapshot {
    uint32_t tick;
    uint32_t timestamp;
    std::vector<EntitySnapshot> entities;
};

// Main game simulation class
class Game {
public:
    Game();
    ~Game() = default;

    // Initialize the game world
    void initialize();

    // Update game simulation by one tick
    void tick(float deltaTime);

    // Process player input
    void processInput(const PlayerInput& input);

    // Get current game state snapshot
    GameSnapshot getSnapshot() const;

    // Get the registry for direct access (mainly for testing)
    registry& getRegistry() { return _registry; }
    const registry& getRegistry() const { return _registry; }

    // Spawn a player entity
    Entity spawnPlayer(uint8_t playerId, float x, float y);

    // Get current tick number
    uint32_t getCurrentTick() const { return _currentTick; }

private:
    registry _registry;
    uint32_t _currentTick;
    uint32_t _nextNetworkId;
    
    // Map entity to network ID
    std::vector<std::pair<Entity, uint32_t>> _entityToNetworkId;
    
    // Track player entities
    std::vector<Entity> _playerEntities;

    void setupSystems();
    uint32_t assignNetworkId(Entity e);
    uint32_t getNetworkId(Entity e) const;
};

} // namespace game
