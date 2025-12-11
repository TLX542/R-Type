#include "../include/GameServer.hpp"
#include "../include/Protocol.hpp"
#include <iostream>
#include <cstring>

GameServer::GameServer(Server* networkServer, unsigned int tickRate)
    : _networkServer(networkServer)
    , _tickRate(tickRate)
    , _running(false)
{
    std::cout << "[GameServer] Initialized with tick rate: " << _tickRate << " Hz" << std::endl;
}

GameServer::~GameServer() {
    stop();
}

void GameServer::start() {
    if (_running) {
        std::cerr << "[GameServer] Already running" << std::endl;
        return;
    }

    _game.initialize();
    _running = true;
    _lastTickTime = std::chrono::steady_clock::now();
    
    _gameThread = std::make_unique<std::thread>(&GameServer::gameLoop, this);
    std::cout << "[GameServer] Game loop started" << std::endl;
}

void GameServer::stop() {
    if (!_running) return;
    
    _running = false;
    if (_gameThread && _gameThread->joinable()) {
        _gameThread->join();
    }
    std::cout << "[GameServer] Game loop stopped" << std::endl;
}

void GameServer::gameLoop() {
    const auto tickDuration = std::chrono::microseconds(1000000 / _tickRate);
    
    while (_running) {
        auto frameStart = std::chrono::steady_clock::now();
        
        // Calculate delta time
        float deltaTime = std::chrono::duration<float>(frameStart - _lastTickTime).count();
        _lastTickTime = frameStart;
        
        // Update game simulation
        _game.tick(deltaTime);
        
        // Broadcast state snapshot to all clients every tick
        // TODO: Optimize to broadcast at lower rate (e.g., 20-30 Hz)
        if (_game.getCurrentTick() % 2 == 0) {  // Broadcast at half tick rate
            broadcastSnapshot();
        }
        
        // Sleep to maintain tick rate
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
        
        if (frameDuration < tickDuration) {
            std::this_thread::sleep_for(tickDuration - frameDuration);
        }
    }
}

void GameServer::broadcastSnapshot() {
    game::GameSnapshot snapshot = _game.getSnapshot();
    
    // Send to all connected clients
    const auto& sessions = _networkServer->getSessions();
    for (auto& session : sessions) {
        if (session->getClientInfo().udpInitialized) {
            sendSnapshotToClient(snapshot, session);
        }
    }
}

void GameServer::sendSnapshotToClient(const game::GameSnapshot& snapshot, std::shared_ptr<Session> session) {
    // For now, send a batch update with all entities
    // TODO: Implement delta compression and proper packet batching
    
    const ClientInfo& clientInfo = session->getClientInfo();
    
    // Send entities in batches of MAX_BATCH_ENTITIES
    size_t numEntities = snapshot.entities.size();
    size_t numBatches = (numEntities + MAX_BATCH_ENTITIES - 1) / MAX_BATCH_ENTITIES;
    
    for (size_t batchIdx = 0; batchIdx < numBatches; ++batchIdx) {
        size_t startIdx = batchIdx * MAX_BATCH_ENTITIES;
        size_t endIdx = std::min(startIdx + MAX_BATCH_ENTITIES, numEntities);
        size_t batchSize = endIdx - startIdx;
        
        // Prepare packet header
        PacketHeader header;
        header.type = ENTITY_BATCH_UPDATE;
        header.sessionToken = clientInfo.sessionToken;
        
        // Prepare payload
        uint8_t buffer[sizeof(PacketHeader) + sizeof(EntityBatchUpdatePayload)];
        std::memset(buffer, 0, sizeof(buffer));
        
        // Copy header
        std::memcpy(buffer, &header, sizeof(PacketHeader));
        
        // Prepare batch payload
        EntityBatchUpdatePayload payload;
        payload.count = static_cast<uint8_t>(batchSize);
        
        for (size_t i = 0; i < batchSize; ++i) {
            const auto& ent = snapshot.entities[startIdx + i];
            payload.entities[i].networkId = ent.networkId;
            payload.entities[i].posX = ent.posX;
            payload.entities[i].posY = ent.posY;
            payload.entities[i].health = ent.health;
        }
        
        // Calculate actual payload size (only send used entries)
        size_t payloadSize = sizeof(uint8_t) + batchSize * sizeof(EntityBatchEntry);
        header.payloadSize = static_cast<uint8_t>(payloadSize);
        
        // Update header in buffer
        std::memcpy(buffer, &header, sizeof(PacketHeader));
        
        // Copy payload
        std::memcpy(buffer + sizeof(PacketHeader), &payload, payloadSize);
        
        // Send via UDP
        try {
            _networkServer->getUdpSocket().send_to(
                asio::buffer(buffer, sizeof(PacketHeader) + payloadSize),
                clientInfo.udpEndpoint
            );
        } catch (std::exception& e) {
            std::cerr << "[GameServer] Error sending snapshot to client: " << e.what() << std::endl;
        }
    }
}

void GameServer::handlePlayerInput(uint8_t playerId, int8_t moveX, int8_t moveY, uint8_t buttons, uint32_t timestamp) {
    game::PlayerInput input;
    input.playerId = playerId;
    input.moveX = moveX;
    input.moveY = moveY;
    input.buttons = buttons;
    input.timestamp = timestamp;
    
    _game.processInput(input);
}

void GameServer::addPlayer(uint8_t playerId) {
    // Spawn player at appropriate position
    float x = 100.0f + playerId * 100.0f;
    float y = 100.0f;
    _game.spawnPlayer(playerId, x, y);
    
    std::cout << "[GameServer] Added player " << (int)playerId << std::endl;
}

void GameServer::removePlayer(uint8_t playerId) {
    // TODO: Remove player entity from game
    std::cout << "[GameServer] Removed player " << (int)playerId << std::endl;
}
