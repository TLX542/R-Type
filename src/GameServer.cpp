#include "../include/GameServer.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <ctime>

GameServer::GameServer(asio::io_context& io_context, short tcpPort, short udpPort)
    : Server(io_context, tcpPort, udpPort),
      _nextNetworkId(1),
      _enemySpawnTimer(0.0f),
      _nextEnemySpawnTime(3.0f),
      _gameRunning(false) {

    // Register component storages
    _registry.register_component<Position>();
    _registry.register_component<Velocity>();
    _registry.register_component<Drawable>();
    _registry.register_component<NetworkId>();
    _registry.register_component<PlayerOwner>();
    _registry.register_component<Health>();
    _registry.register_component<Damage>();
    _registry.register_component<EntityTypeTag>();
    _registry.register_component<Lifetime>();

    std::cout << "[GameServer] ECS initialized with gameplay components" << std::endl;
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
    // Enemy spawning system
    _enemySpawnTimer += deltaTime;
    if (_enemySpawnTimer >= _nextEnemySpawnTime) {
        spawnEnemy();
        _enemySpawnTimer = 0.0f;
        // Random next spawn time between 3 and 5 seconds
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(MIN_ENEMY_SPAWN_INTERVAL, MAX_ENEMY_SPAWN_INTERVAL);
        _nextEnemySpawnTime = dist(gen);
    }

    // Update lifetimes (bullets auto-despawn)
    updateLifetimes(deltaTime);

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

        // Simple boundary check (800x600 game area) - only for players
        if (pos.x < 0) pos.x = 0;
        if (pos.x > 800) pos.x = 800;
        if (pos.y < 0) pos.y = 0;
        if (pos.y > 600) pos.y = 600;
    }

    // Check collisions
    checkCollisions();
}

void GameServer::broadcastWorldState() {
    // Get component storages
    auto* positions = _registry.get_components_if<Position>();
    auto* networkIds = _registry.get_components_if<NetworkId>();
    auto* drawables = _registry.get_components_if<Drawable>();
    auto* healths = _registry.get_components_if<Health>();

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

        // Get health if available, default to 100
        uint8_t health = 100;
        if (healths) {
            auto& health_opt = healths->get_ref(i);
            if (health_opt) {
                health = health_opt.value().current;
            }
        }

        // Add to batch
        EntityBatchEntry& entry = batchPayload.entities[batchPayload.count];
        entry.networkId = netId.id;
        entry.posX = pos.x;
        entry.posY = pos.y;
        entry.health = health;

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
        int sentCount = 0;
        for (auto& session : _sessions) {
            if (session->getClientInfo().udpInitialized) {
                _udpSocket.send_to(asio::buffer(packet), session->getClientInfo().udpEndpoint);
                sentCount++;
            }
        }
        
        // Log occasionally (every 60 updates = ~1 second)
        static int updateCounter = 0;
        updateCounter++;
        if (updateCounter % 60 == 0) {
            std::cout << "[GameServer] Broadcast update " << updateCounter 
                      << ": " << (int)batchPayload.count << " entities to " 
                      << sentCount << " clients" << std::endl;
        }
    }
}

void GameServer::handlePlayerInput(uint8_t playerId, int8_t moveX, int8_t moveY, uint8_t buttons) {
    // Find player entity
    auto it = _playerEntities.find(playerId);
    if (it == _playerEntities.end()) {
        std::cerr << "[GameServer] WARNING: Received input for unknown player " << (int)playerId << std::endl;
        return;
    }

    Entity playerEntity = it->second;

    // Only log if there's actual input (not idle)
    if (moveX != 0 || moveY != 0 || buttons != 0) {
        std::cout << "[GameServer] Player " << (int)playerId
                  << " input: move(" << (int)moveX << "," << (int)moveY << ")"
                  << " buttons=" << (int)buttons
                  << " entity=" << static_cast<size_t>(playerEntity) << std::endl;
    }

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

    // Handle shooting button
    const uint8_t BTN_SHOOT = 0x01;
    if (buttons & BTN_SHOOT) {
        spawnBullet(playerId, playerEntity);
    }
}

void GameServer::onPlayerConnected(uint8_t playerId) {
    std::cout << "[GameServer] Player " << (int)playerId << " connected (TCP)" << std::endl;
    std::cout << "[GameServer] Creating entity for player " << (int)playerId
              << " (will spawn when UDP ready)" << std::endl;

    // Spawn a player entity
    Entity playerEntity = _registry.spawn_entity();
    std::cout << "[GameServer] Created entity ID=" << static_cast<size_t>(playerEntity)
              << " for player " << (int)playerId << std::endl;

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
    _registry.add_component<Health>(playerEntity, Health{100, 100});
    _registry.add_component<EntityTypeTag>(playerEntity, EntityTypeTag{EntityTypeTag::PLAYER});

    // Store mapping
    _playerEntities.insert({ playerId, playerEntity });
    
    std::cout << "[GameServer] Entity created for player " << (int)playerId << std::endl;
}

void GameServer::onPlayerUdpReady(uint8_t playerId) {
    std::cout << "[GameServer] Player " << (int)playerId << " UDP ready, sending ENTITY_SPAWN" << std::endl;

    // Find the session for this player
    Session* newPlayerSession = nullptr;
    for (auto& session : _sessions) {
        if (session->getId() == playerId) {
            newPlayerSession = session.get();
            break;
        }
    }

    if (!newPlayerSession) {
        std::cerr << "[GameServer] ERROR: No session found for player " << (int)playerId << std::endl;
        return;
    }

    auto& newPlayerEndpoint = newPlayerSession->getClientInfo().udpEndpoint;

    auto* networkIds = _registry.get_components_if<NetworkId>();
    auto* positions = _registry.get_components_if<Position>();
    auto* velocities = _registry.get_components_if<Velocity>();
    auto* healths = _registry.get_components_if<Health>();
    auto* types = _registry.get_components_if<EntityTypeTag>();
    auto* playerOwners = _registry.get_components_if<PlayerOwner>();

    if (!networkIds || !positions || !velocities || !types || !playerOwners) {
        std::cerr << "[GameServer] ERROR: Missing component storages" << std::endl;
        return;
    }

    // Helper lambda to send ENTITY_SPAWN for a given entity to the new player
    auto sendSpawnToNewPlayer = [&](Entity entity) {
        size_t idx = static_cast<size_t>(entity);
        auto& netId_opt = networkIds->get_ref(idx);
        auto& pos_opt = positions->get_ref(idx);
        auto& vel_opt = velocities->get_ref(idx);
        auto& type_opt = types->get_ref(idx);
        auto& owner_opt = playerOwners->get_ref(idx);

        if (!netId_opt || !pos_opt || !vel_opt || !type_opt || !owner_opt) {
            return;
        }

        EntitySpawnPayload spawnPayload;
        spawnPayload.networkId = netId_opt.value().id;
        spawnPayload.entityType = type_opt.value().type;
        spawnPayload.ownerPlayer = owner_opt.value().playerId;
        spawnPayload.posX = pos_opt.value().x;
        spawnPayload.posY = pos_opt.value().y;
        spawnPayload.velocityX = vel_opt.value().vx;
        spawnPayload.velocityY = vel_opt.value().vy;

        // Get health if available
        spawnPayload.health = 100;
        if (healths) {
            auto& health_opt = healths->get_ref(idx);
            if (health_opt) {
                spawnPayload.health = health_opt.value().current;
            }
        }

        // Get username for PLAYER entities
        std::memset(spawnPayload.username, 0, sizeof(spawnPayload.username));
        if (type_opt.value().type == EntityTypeTag::PLAYER && owner_opt.value().playerId != 0) {
            // Find the session for this player
            for (auto& session : _sessions) {
                if (session->getId() == owner_opt.value().playerId) {
                    const std::string& username = session->getClientInfo().username;
                    std::strncpy(spawnPayload.username, username.c_str(), sizeof(spawnPayload.username) - 1);
                    break;
                }
            }
        }

        PacketHeader header;
        header.type = ENTITY_SPAWN;
        header.payloadSize = sizeof(EntitySpawnPayload);
        header.sessionToken = 0;

        std::vector<char> packet(sizeof(PacketHeader) + sizeof(EntitySpawnPayload));
        std::memcpy(packet.data(), &header, sizeof(PacketHeader));
        std::memcpy(packet.data() + sizeof(PacketHeader), &spawnPayload, sizeof(EntitySpawnPayload));

        _udpSocket.send_to(asio::buffer(packet), newPlayerEndpoint);

        std::cout << "[GameServer] Sent ENTITY_SPAWN to new player " << (int)playerId
                  << " for network ID " << spawnPayload.networkId
                  << " (type=" << (int)spawnPayload.entityType << ")" << std::endl;
    };

    // Send ENTITY_SPAWN for ALL existing players to the new player
    std::cout << "[GameServer] Sending existing player entities to player " << (int)playerId << std::endl;
    for (auto& pair : _playerEntities) {
        sendSpawnToNewPlayer(pair.second);
    }

    // Send ENTITY_SPAWN for ALL existing enemies to the new player
    std::cout << "[GameServer] Sending existing enemy entities to player " << (int)playerId << std::endl;
    for (auto enemy : _enemyEntities) {
        sendSpawnToNewPlayer(enemy);
    }

    // Send ENTITY_SPAWN for ALL existing bullets to the new player
    std::cout << "[GameServer] Sending existing bullet entities to player " << (int)playerId << std::endl;
    for (auto bullet : _bulletEntities) {
        sendSpawnToNewPlayer(bullet);
    }

    // Now broadcast THIS new player's entity to ALL clients (including themselves)
    auto it = _playerEntities.find(playerId);
    if (it != _playerEntities.end()) {
        Entity playerEntity = it->second;
        size_t idx = static_cast<size_t>(playerEntity);
        auto& netId_opt = networkIds->get_ref(idx);
        auto& pos_opt = positions->get_ref(idx);
        auto& vel_opt = velocities->get_ref(idx);

        if (netId_opt && pos_opt && vel_opt) {
            EntitySpawnPayload spawnPayload;
            spawnPayload.networkId = netId_opt.value().id;
            spawnPayload.entityType = PLAYER;
            spawnPayload.ownerPlayer = playerId;
            spawnPayload.posX = pos_opt.value().x;
            spawnPayload.posY = pos_opt.value().y;
            spawnPayload.velocityX = vel_opt.value().vx;
            spawnPayload.velocityY = vel_opt.value().vy;
            spawnPayload.health = 100;

            // Set username
            std::memset(spawnPayload.username, 0, sizeof(spawnPayload.username));
            std::strncpy(spawnPayload.username, newPlayerSession->getClientInfo().username.c_str(),
                         sizeof(spawnPayload.username) - 1);

            PacketHeader header;
            header.type = ENTITY_SPAWN;
            header.payloadSize = sizeof(EntitySpawnPayload);
            header.sessionToken = 0;

            std::vector<char> packet(sizeof(PacketHeader) + sizeof(EntitySpawnPayload));
            std::memcpy(packet.data(), &header, sizeof(PacketHeader));
            std::memcpy(packet.data() + sizeof(PacketHeader), &spawnPayload, sizeof(EntitySpawnPayload));

            std::cout << "[GameServer] Broadcasting new player " << (int)playerId
                      << " (network ID " << spawnPayload.networkId << ", username: "
                      << spawnPayload.username << ") to all clients" << std::endl;

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

// Gameplay systems implementation

void GameServer::spawnBullet(uint8_t playerId, Entity playerEntity) {
    // Simple rate limiting: track last shoot time per player
    static std::unordered_map<uint8_t, std::chrono::steady_clock::time_point> lastShootTime;
    auto now = std::chrono::steady_clock::now();
    auto it = lastShootTime.find(playerId);

    const auto SHOOT_COOLDOWN = std::chrono::milliseconds(250);  // 4 shots per second max

    if (it != lastShootTime.end()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second);
        if (elapsed < SHOOT_COOLDOWN) {
            return;  // Too soon, ignore
        }
    }

    lastShootTime[playerId] = now;

    // Get player position
    auto* positions = _registry.get_components_if<Position>();
    if (!positions) return;

    auto& pos_opt = positions->get_ref(static_cast<size_t>(playerEntity));
    if (!pos_opt) return;

    auto& playerPos = pos_opt.value();

    // Create bullet entity
    Entity bullet = _registry.spawn_entity();

    // Spawn bullet slightly in front of player
    float bulletX = playerPos.x + 60.0f;  // Offset to the right
    float bulletY = playerPos.y + 20.0f;  // Center of player

    _registry.add_component<Position>(bullet, Position{bulletX, bulletY});
    _registry.add_component<Velocity>(bullet, Velocity{400.0f, 0.0f});  // Fast moving right
    _registry.add_component<Drawable>(bullet, Drawable{8.0f, 2.0f, Color{255, 255, 0}});  // Yellow bullet (thin)
    _registry.add_component<NetworkId>(bullet, NetworkId{_nextNetworkId++});
    _registry.add_component<PlayerOwner>(bullet, PlayerOwner{playerId});
    _registry.add_component<EntityTypeTag>(bullet, EntityTypeTag{EntityTypeTag::BULLET_PLAYER});
    _registry.add_component<Damage>(bullet, Damage{25});
    _registry.add_component<Lifetime>(bullet, Lifetime{3.0f});  // Despawn after 3 seconds

    _bulletEntities.push_back(bullet);

    // Broadcast ENTITY_SPAWN to all clients
    EntitySpawnPayload spawnPayload;
    spawnPayload.networkId = _nextNetworkId - 1;
    spawnPayload.entityType = BULLET_PLAYER;
    spawnPayload.ownerPlayer = playerId;
    spawnPayload.posX = bulletX;
    spawnPayload.posY = bulletY;
    spawnPayload.velocityX = 400.0f;
    spawnPayload.velocityY = 0.0f;
    spawnPayload.health = 1;
    std::memset(spawnPayload.username, 0, sizeof(spawnPayload.username));  // Empty for bullets

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

void GameServer::spawnEnemy() {
    Entity enemy = _registry.spawn_entity();

    // Spawn at random Y position on the right edge
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> yDist(0.0f, 600.0f);

    float enemyX = 850.0f;  // Just off right edge
    float enemyY = yDist(gen);

    _registry.add_component<Position>(enemy, Position{enemyX, enemyY});
    _registry.add_component<Velocity>(enemy, Velocity{-150.0f, 0.0f});  // Move left at fixed speed
    _registry.add_component<Drawable>(enemy, Drawable{40.0f, 40.0f, Color{255, 0, 0}});  // Red enemy
    _registry.add_component<NetworkId>(enemy, NetworkId{_nextNetworkId++});
    _registry.add_component<PlayerOwner>(enemy, PlayerOwner{0});  // Server-owned
    _registry.add_component<EntityTypeTag>(enemy, EntityTypeTag{EntityTypeTag::ENEMY});
    _registry.add_component<Health>(enemy, Health{50, 50});

    _enemyEntities.push_back(enemy);

    // Broadcast ENTITY_SPAWN to all clients
    EntitySpawnPayload spawnPayload;
    spawnPayload.networkId = _nextNetworkId - 1;
    spawnPayload.entityType = ENEMY;
    spawnPayload.ownerPlayer = 0;
    spawnPayload.posX = enemyX;
    spawnPayload.posY = enemyY;
    spawnPayload.velocityX = -150.0f;
    spawnPayload.velocityY = 0.0f;
    spawnPayload.health = 50;
    std::memset(spawnPayload.username, 0, sizeof(spawnPayload.username));  // Empty for enemies

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

    std::cout << "[GameServer] Spawned enemy at (" << enemyX << ", " << enemyY << ")" << std::endl;
}

void GameServer::updateLifetimes(float deltaTime) {
    auto* lifetimes = _registry.get_components_if<Lifetime>();
    if (!lifetimes) return;

    std::vector<Entity> toDestroy;

    for (std::size_t i = 0; i < lifetimes->size(); ++i) {
        auto& lifetime_opt = lifetimes->get_ref(i);
        if (!lifetime_opt) continue;

        auto& lifetime = lifetime_opt.value();
        lifetime.remaining -= deltaTime;

        if (lifetime.remaining <= 0.0f) {
            toDestroy.push_back(_registry.entity_from_index(i));
        }
    }

    // Destroy entities that expired
    for (auto entity : toDestroy) {
        destroyEntity(entity);
    }
}

void GameServer::checkCollisions() {
    auto* positions = _registry.get_components_if<Position>();
    auto* drawables = _registry.get_components_if<Drawable>();
    auto* types = _registry.get_components_if<EntityTypeTag>();
    auto* damages = _registry.get_components_if<Damage>();
    auto* healths = _registry.get_components_if<Health>();

    if (!positions || !drawables || !types || !healths) return;

    std::vector<Entity> toDestroy;

    // Check bullet vs enemy collisions
    for (auto bullet : _bulletEntities) {
        size_t bulletIdx = static_cast<size_t>(bullet);
        auto& bullet_pos_opt = positions->get_ref(bulletIdx);
        auto& bullet_draw_opt = drawables->get_ref(bulletIdx);
        auto& bullet_type_opt = types->get_ref(bulletIdx);

        if (!bullet_pos_opt || !bullet_draw_opt || !bullet_type_opt) continue;
        if (bullet_type_opt.value().type != EntityTypeTag::BULLET_PLAYER) continue;

        auto& bulletPos = bullet_pos_opt.value();
        auto& bulletDraw = bullet_draw_opt.value();

        // Get bullet damage
        uint8_t bulletDamage = 25;
        if (damages) {
            auto& damage_opt = damages->get_ref(bulletIdx);
            if (damage_opt) {
                bulletDamage = damage_opt.value().amount;
            }
        }

        // Check against all enemies
        for (auto enemy : _enemyEntities) {
            size_t enemyIdx = static_cast<size_t>(enemy);
            auto& enemy_pos_opt = positions->get_ref(enemyIdx);
            auto& enemy_draw_opt = drawables->get_ref(enemyIdx);
            auto& enemy_health_opt = healths->get_ref(enemyIdx);

            if (!enemy_pos_opt || !enemy_draw_opt || !enemy_health_opt) continue;

            auto& enemyPos = enemy_pos_opt.value();
            auto& enemyDraw = enemy_draw_opt.value();
            auto& enemyHealth = enemy_health_opt.value();

            // Simple AABB collision detection
            bool collision = (bulletPos.x < enemyPos.x + enemyDraw.width &&
                              bulletPos.x + bulletDraw.width > enemyPos.x &&
                              bulletPos.y < enemyPos.y + enemyDraw.height &&
                              bulletPos.y + bulletDraw.height > enemyPos.y);

            if (collision) {
                // Apply damage
                if (enemyHealth.current > bulletDamage) {
                    enemyHealth.current -= bulletDamage;
                } else {
                    enemyHealth.current = 0;
                    toDestroy.push_back(enemy);
                }

                // Destroy bullet
                toDestroy.push_back(bullet);
                break;  // Bullet can only hit one enemy
            }
        }
    }

    // Destroy off-screen enemies (left edge)
    for (auto enemy : _enemyEntities) {
        size_t enemyIdx = static_cast<size_t>(enemy);
        auto& enemy_pos_opt = positions->get_ref(enemyIdx);
        if (!enemy_pos_opt) continue;

        auto& enemyPos = enemy_pos_opt.value();
        if (enemyPos.x < -100.0f) {  // Off left edge
            toDestroy.push_back(enemy);
        }
    }

    // Destroy off-screen bullets (right edge)
    for (auto bullet : _bulletEntities) {
        size_t bulletIdx = static_cast<size_t>(bullet);
        auto& bullet_pos_opt = positions->get_ref(bulletIdx);
        if (!bullet_pos_opt) continue;

        auto& bulletPos = bullet_pos_opt.value();
        if (bulletPos.x > 900.0f) {  // Off right edge
            toDestroy.push_back(bullet);
        }
    }

    // Remove duplicates
    std::sort(toDestroy.begin(), toDestroy.end());
    toDestroy.erase(std::unique(toDestroy.begin(), toDestroy.end()), toDestroy.end());

    // Destroy all marked entities
    for (auto entity : toDestroy) {
        destroyEntity(entity);
    }
}

void GameServer::destroyEntity(Entity entity) {
    // Get network ID before destroying
    auto* networkIds = _registry.get_components_if<NetworkId>();
    uint32_t networkId = 0;

    if (networkIds) {
        auto& netId_opt = networkIds->get_ref(static_cast<size_t>(entity));
        if (netId_opt) {
            networkId = netId_opt.value().id;
        }
    }

    // Remove from tracking containers
    _enemyEntities.erase(
        std::remove(_enemyEntities.begin(), _enemyEntities.end(), entity),
        _enemyEntities.end()
    );
    _bulletEntities.erase(
        std::remove(_bulletEntities.begin(), _bulletEntities.end(), entity),
        _bulletEntities.end()
    );

    // Destroy entity in ECS
    _registry.kill_entity(entity);

    // Broadcast ENTITY_DESTROY to all clients
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
