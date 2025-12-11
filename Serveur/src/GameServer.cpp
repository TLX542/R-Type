#include "../include/GameServer.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>

GameServer::GameServer(asio::io_context& io_context, short tcpPort, short udpPort)
    : Server(io_context, tcpPort, udpPort),
      _nextNetworkId(1),
      _gameRunning(false) {
    
    // Register component storages
    _registry.register_component<Position>();
    _registry.register_component<Velocity>();
    _registry.register_component<Drawable>();
    _registry.register_component<NetworkId>();
    _registry.register_component<PlayerOwner>();

    std::cout << "[GameServer] ECS initialized" << std::endl;
}

GameServer::~GameServer() {
    stopGameLoop();
}

void GameServer::startGameLoop() {
    if (_gameRunning) {
        return;
    }

    _gameRunning = true;
    _gameThread = std::thread(&GameServer::gameLoopThread, this);
    std::cout << "[GameServer] Game loop started" << std::endl;
}

void GameServer::stopGameLoop() {
    if (!_gameRunning) {
        return;
    }

    _gameRunning = false;
    if (_gameThread.joinable()) {
        _gameThread.join();
    }
    std::cout << "[GameServer] Game loop stopped" << std::endl;
}

void GameServer::gameLoopThread() {
    using clock = std::chrono::high_resolution_clock;
    auto lastUpdate = clock::now();

    while (_gameRunning) {
        auto now = clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastUpdate).count();

        if (deltaTime >= TICK_INTERVAL) {
            updateGame(deltaTime);
            broadcastWorldState();
            lastUpdate = now;
        }

        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameServer::updateGame(float deltaTime) {
    // Get component storages
    auto* positions = _registry.get_components_if<Position>();
    auto* velocities = _registry.get_components_if<Velocity>();

    if (!positions || !velocities) {
        return;
    }

    // Update positions based on velocities (position system)
    std::size_t limit = std::min(positions->size(), velocities->size());
    for (std::size_t i = 0; i < limit; ++i) {
        auto& pos_opt = positions->get_ref(i);
        auto& vel_opt = velocities->get_ref(i);
        
        if (!pos_opt || !vel_opt) continue;
        
        auto& pos = pos_opt.value();
        auto& vel = vel_opt.value();
        
        // Update position
        pos.x += vel.vx * deltaTime;
        pos.y += vel.vy * deltaTime;

        // Simple boundary check (800x600 game area)
        if (pos.x < 0) pos.x = 0;
        if (pos.x > 800) pos.x = 800;
        if (pos.y < 0) pos.y = 0;
        if (pos.y > 600) pos.y = 600;
    }
}

void GameServer::broadcastWorldState() {
    // Get component storages
    auto* positions = _registry.get_components_if<Position>();
    auto* networkIds = _registry.get_components_if<NetworkId>();
    auto* drawables = _registry.get_components_if<Drawable>();

    if (!positions || !networkIds || !drawables) {
        return;
    }

    // Prepare batch update
    EntityBatchUpdatePayload batchPayload;
    batchPayload.count = 0;

    std::size_t limit = std::min({positions->size(), networkIds->size(), drawables->size()});
    
    for (std::size_t i = 0; i < limit && batchPayload.count < MAX_BATCH_ENTITIES; ++i) {
        auto& pos_opt = positions->get_ref(i);
        auto& netId_opt = networkIds->get_ref(i);
        auto& draw_opt = drawables->get_ref(i);
        
        if (!pos_opt || !netId_opt || !draw_opt) continue;
        
        auto& pos = pos_opt.value();
        auto& netId = netId_opt.value();
        
        // Add to batch
        EntityBatchEntry& entry = batchPayload.entities[batchPayload.count];
        entry.networkId = netId.id;
        entry.posX = pos.x;
        entry.posY = pos.y;
        entry.health = 100;  // Simplified - no health system yet
        
        batchPayload.count++;
    }

    // Send batch update to all clients if we have entities
    if (batchPayload.count > 0) {
        // Prepare packet
        PacketHeader header;
        header.type = ENTITY_BATCH_UPDATE;
        header.payloadSize = sizeof(uint8_t) + batchPayload.count * sizeof(EntityBatchEntry);
        header.sessionToken = 0;  // Broadcast to all

        // Build packet buffer
        std::vector<char> packet(sizeof(PacketHeader) + header.payloadSize);
        std::memcpy(packet.data(), &header, sizeof(PacketHeader));
        std::memcpy(packet.data() + sizeof(PacketHeader), &batchPayload.count, sizeof(uint8_t));
        std::memcpy(packet.data() + sizeof(PacketHeader) + sizeof(uint8_t), 
                    batchPayload.entities, 
                    batchPayload.count * sizeof(EntityBatchEntry));

        // Send to all connected clients
        for (auto& session : _sessions) {
            if (session->getClientInfo().udpInitialized) {
                _udpSocket.send_to(asio::buffer(packet), session->getClientInfo().udpEndpoint);
            }
        }
    }
}

void GameServer::handlePlayerInput(uint8_t playerId, int8_t moveX, int8_t moveY, uint8_t buttons) {
    // Find player entity
    auto it = _playerEntities.find(playerId);
    if (it == _playerEntities.end()) {
        return;
    }

    Entity playerEntity = it->second;
    
    // Get velocity component storage
    auto* velocities = _registry.get_components_if<Velocity>();
    if (!velocities) {
        return;
    }

    // Update velocity based on input
    auto& vel_opt = velocities->get_ref(static_cast<size_t>(playerEntity));
    if (vel_opt) {
        auto& vel = vel_opt.value();
        
        // Apply movement (speed of 200 pixels per second)
        const float PLAYER_SPEED = 200.0f;
        vel.vx = moveX * PLAYER_SPEED;
        vel.vy = moveY * PLAYER_SPEED;
    }

    // TODO: Handle buttons (shooting, etc.)
    (void)buttons;
}

void GameServer::onPlayerConnected(uint8_t playerId) {
    std::cout << "[GameServer] Spawning entity for player " << (int)playerId << std::endl;

    // Spawn a player entity
    Entity playerEntity = _registry.spawn_entity();

    // Add components
    float startX = 100.0f + playerId * 100.0f;  // Spread players horizontally
    float startY = 300.0f;

    _registry.add_component<Position>(playerEntity, Position{startX, startY});
    _registry.add_component<Velocity>(playerEntity, Velocity{0.0f, 0.0f});
    
    // Different color for each player
    Color playerColors[] = {
        Color{200, 30, 30},    // Red
        Color{30, 200, 30},    // Green
        Color{30, 30, 200},    // Blue
        Color{200, 200, 30}    // Yellow
    };
    Color color = playerColors[(playerId - 1) % 4];
    _registry.add_component<Drawable>(playerEntity, Drawable{48.0f, 48.0f, color});
    
    _registry.add_component<NetworkId>(playerEntity, NetworkId{_nextNetworkId++});
    _registry.add_component<PlayerOwner>(playerEntity, PlayerOwner{playerId});

    // Store mapping
    _playerEntities.insert({ playerId, playerEntity });

    // Send ENTITY_SPAWN to all clients
    EntitySpawnPayload spawnPayload;
    auto* networkIds = _registry.get_components_if<NetworkId>();
    auto* positions = _registry.get_components_if<Position>();
    auto* velocities = _registry.get_components_if<Velocity>();
    auto* drawables = _registry.get_components_if<Drawable>();

    if (networkIds && positions && velocities && drawables) {
        size_t idx = static_cast<size_t>(playerEntity);
        auto& netId_opt = networkIds->get_ref(idx);
        auto& pos_opt = positions->get_ref(idx);
        auto& vel_opt = velocities->get_ref(idx);
        auto& draw_opt = drawables->get_ref(idx);

        if (netId_opt && pos_opt && vel_opt && draw_opt) {
            spawnPayload.networkId = netId_opt.value().id;
            spawnPayload.entityType = PLAYER;
            spawnPayload.ownerPlayer = playerId;
            spawnPayload.posX = pos_opt.value().x;
            spawnPayload.posY = pos_opt.value().y;
            spawnPayload.velocityX = vel_opt.value().vx;
            spawnPayload.velocityY = vel_opt.value().vy;
            spawnPayload.health = 100;

            // Broadcast spawn to all clients
            PacketHeader header;
            header.type = ENTITY_SPAWN;
            header.payloadSize = sizeof(EntitySpawnPayload);
            header.sessionToken = 0;

            std::vector<char> packet(sizeof(PacketHeader) + sizeof(EntitySpawnPayload));
            std::memcpy(packet.data(), &header, sizeof(PacketHeader));
            std::memcpy(packet.data() + sizeof(PacketHeader), &spawnPayload, sizeof(EntitySpawnPayload));

            for (auto& session : _sessions) {
                if (session->getClientInfo().udpInitialized) {
                    _udpSocket.send_to(asio::buffer(packet), session->getClientInfo().udpEndpoint);
                }
            }
        }
    }
}

void GameServer::onPlayerDisconnected(uint8_t playerId) {
    auto it = _playerEntities.find(playerId);
    if (it == _playerEntities.end()) {
        return;
    }

    Entity playerEntity = it->second;
    
    // Get network ID before destroying
    auto* networkIds = _registry.get_components_if<NetworkId>();
    uint32_t networkId = 0;
    
    if (networkIds) {
        auto& netId_opt = networkIds->get_ref(static_cast<size_t>(playerEntity));
        if (netId_opt) {
            networkId = netId_opt.value().id;
        }
    }

    // Destroy entity
    _registry.kill_entity(playerEntity);
    _playerEntities.erase(it);

    std::cout << "[GameServer] Destroyed entity for player " << (int)playerId << std::endl;

    // Send ENTITY_DESTROY to all clients
    if (networkId != 0) {
        EntityDestroyPayload destroyPayload;
        destroyPayload.networkId = networkId;

        PacketHeader header;
        header.type = ENTITY_DESTROY;
        header.payloadSize = sizeof(EntityDestroyPayload);
        header.sessionToken = 0;

        std::vector<char> packet(sizeof(PacketHeader) + sizeof(EntityDestroyPayload));
        std::memcpy(packet.data(), &header, sizeof(PacketHeader));
        std::memcpy(packet.data() + sizeof(PacketHeader), &destroyPayload, sizeof(EntityDestroyPayload));

        for (auto& session : _sessions) {
            if (session->getClientInfo().udpInitialized) {
                _udpSocket.send_to(asio::buffer(packet), session->getClientInfo().udpEndpoint);
            }
        }
    }
}
