#include "Game.hpp"
#include <algorithm>
#include <iostream>

namespace game {

Game::Game() 
    : _currentTick(0)
    , _nextNetworkId(1)
{
}

void Game::initialize() {
    // Register components
    _registry.register_component<Position>();
    _registry.register_component<Velocity>();
    _registry.register_component<Drawable>();
    _registry.register_component<Controllable>();

    setupSystems();

    // Spawn some initial entities for testing
    // These can be removed later or made configurable
    for (int i = 0; i < 3; ++i) {
        auto e = _registry.spawn_entity();
        float x = 100.0f + i * 150.0f;
        float y = 350.0f;
        _registry.add_component<Position>(e, Position{x, y});
        _registry.add_component<Drawable>(e, Drawable{64.0f, 32.0f, Color{30, static_cast<uint8_t>(100 + i * 30), 200}});
        assignNetworkId(e);
    }

    std::cout << "[Game] Initialized with " << _entityToNetworkId.size() << " entities" << std::endl;
}

void Game::setupSystems() {
    // Position integration system: updates position based on velocity
    _registry.add_system<Position, Velocity>([](registry& /*r*/, HybridArray<Position>& pos, HybridArray<Velocity>& vel) {
        std::size_t limit = std::min(pos.size(), vel.size());
        for (std::size_t i = 0; i < limit; ++i) {
            auto &pos_opt = pos.get_ref(i);
            auto &vel_opt = vel.get_ref(i);
            if (!pos_opt || !vel_opt) continue;
            auto & p = pos_opt.value();
            auto const & v = vel_opt.value();
            p.x += v.vx;
            p.y += v.vy;
        }
    });

    // TODO: Add collision detection system
    // TODO: Add boundary checking system
}

void Game::tick(float deltaTime) {
    // Run all registered systems
    _registry.run_systems();
    
    ++_currentTick;
    
    // Optional: Print debug info every N ticks
    if (_currentTick % 600 == 0) {  // Every 10 seconds at 60Hz
        std::cout << "[Game] Tick " << _currentTick << ", entities: " << _entityToNetworkId.size() << std::endl;
    }
}

void Game::processInput(const PlayerInput& input) {
    // Find the player entity for this player ID
    if (input.playerId >= _playerEntities.size()) {
        // Player doesn't exist yet, ignore
        return;
    }

    Entity playerEntity = _playerEntities[input.playerId];
    
    // Update velocity based on input
    auto* velocities = _registry.get_components_if<Velocity>();
    auto* controllables = _registry.get_components_if<Controllable>();
    
    if (velocities && controllables) {
        std::size_t idx = static_cast<std::size_t>(playerEntity);
        auto& vel_opt = velocities->get_ref(idx);
        auto& ctrl_opt = controllables->get_ref(idx);
        
        if (vel_opt && ctrl_opt) {
            auto& vel = vel_opt.value();
            const auto& ctrl = ctrl_opt.value();
            
            // Set velocity based on input direction and controllable speed
            vel.vx = input.moveX * ctrl.speed;
            vel.vy = input.moveY * ctrl.speed;
            
            // TODO: Handle button presses (shoot, special, etc.)
        }
    }
}

Entity Game::spawnPlayer(uint8_t playerId, float x, float y) {
    auto player = _registry.spawn_entity();
    _registry.add_component<Position>(player, Position{x, y});
    _registry.add_component<Velocity>(player, Velocity{0.0f, 0.0f});
    _registry.add_component<Drawable>(player, Drawable{48.0f, 48.0f, Color{200, 30, 30}});
    _registry.add_component<Controllable>(player, Controllable{4.0f});
    
    assignNetworkId(player);
    
    // Ensure player entities vector is large enough
    while (_playerEntities.size() <= playerId) {
        _playerEntities.push_back(_registry.entity_from_index(0));  // Invalid entity as placeholder
    }
    _playerEntities[playerId] = player;
    
    std::cout << "[Game] Spawned player " << (int)playerId << " at (" << x << ", " << y << ")" << std::endl;
    
    return player;
}

GameSnapshot Game::getSnapshot() const {
    GameSnapshot snapshot;
    snapshot.tick = _currentTick;
    snapshot.timestamp = _currentTick * 16;  // Assuming ~60Hz = 16ms per tick
    
    // Collect all entities with position
    const auto* positions = _registry.get_components_if<Position>();
    const auto* drawables = _registry.get_components_if<Drawable>();
    const auto* velocities = _registry.get_components_if<Velocity>();
    
    if (!positions) return snapshot;
    
    for (const auto& [entity, networkId] : _entityToNetworkId) {
        std::size_t idx = static_cast<std::size_t>(entity);
        
        // Use get() method which is const-correct
        auto pos_opt = positions->get(idx);
        if (!pos_opt) continue;
        
        const auto& pos = pos_opt.value();
        
        EntitySnapshot entSnap;
        entSnap.networkId = networkId;
        entSnap.entityType = 0;  // TODO: Proper entity type
        entSnap.posX = pos.x;
        entSnap.posY = pos.y;
        entSnap.velX = 0.0f;
        entSnap.velY = 0.0f;
        
        // Get velocity if available
        if (velocities) {
            auto vel_opt = velocities->get(idx);
            if (vel_opt) {
                const auto& vel = vel_opt.value();
                entSnap.velX = vel.vx;
                entSnap.velY = vel.vy;
            }
        }
        
        // Get drawable properties if available
        if (drawables) {
            auto draw_opt = drawables->get(idx);
            if (draw_opt) {
                const auto& draw = draw_opt.value();
                entSnap.width = draw.width;
                entSnap.height = draw.height;
                entSnap.colorR = draw.color.r;
                entSnap.colorG = draw.color.g;
                entSnap.colorB = draw.color.b;
            } else {
                entSnap.width = 16.0f;
                entSnap.height = 16.0f;
                entSnap.colorR = 255;
                entSnap.colorG = 255;
                entSnap.colorB = 255;
            }
        } else {
            entSnap.width = 16.0f;
            entSnap.height = 16.0f;
            entSnap.colorR = 255;
            entSnap.colorG = 255;
            entSnap.colorB = 255;
        }
        
        entSnap.health = 100;
        entSnap.ownerPlayer = 0;
        
        // Check if this is a player entity
        for (size_t i = 0; i < _playerEntities.size(); ++i) {
            if (_playerEntities[i] == entity) {
                entSnap.entityType = 0;  // Player type
                entSnap.ownerPlayer = static_cast<uint8_t>(i);
                break;
            }
        }
        
        snapshot.entities.push_back(entSnap);
    }
    
    return snapshot;
}

uint32_t Game::assignNetworkId(Entity e) {
    uint32_t id = _nextNetworkId++;
    _entityToNetworkId.push_back({e, id});
    return id;
}

uint32_t Game::getNetworkId(Entity e) const {
    for (const auto& [entity, networkId] : _entityToNetworkId) {
        if (entity == e) return networkId;
    }
    return 0;  // Invalid
}

} // namespace game
