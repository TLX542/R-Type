#pragma once

#include "Server.hpp"
#include "Registry.hpp"
#include "Components.hpp"
#include "HybridArray.hpp"
#include "Entity.hpp"
#include <thread>
#include <atomic>
#include <chrono>
#include <unordered_map>

/**
 * @brief GameServer combines the network server with ECS game logic
 * 
 * This class runs the authoritative game simulation on the server side.
 * It manages:
 * - Network connections (via Server base functionality)
 * - ECS game world (registry, entities, components)
 * - Game loop that updates entities at fixed tick rate
 * - World state synchronization to clients
 */
class GameServer : public Server {
public:
    GameServer(asio::io_context& io_context, short tcpPort, short udpPort);
    ~GameServer();

    void startGameLoop();
    void stopGameLoop();

    // Process player input and apply to ECS
    void handlePlayerInput(uint8_t playerId, int8_t moveX, int8_t moveY, uint8_t buttons);

    // Called when a client connects and is authenticated
    void onPlayerConnected(uint8_t playerId);

    // Called when a client disconnects
    void onPlayerDisconnected(uint8_t playerId);

private:
    // Game loop runs in separate thread
    void gameLoopThread();
    void updateGame(float deltaTime);
    void broadcastWorldState();

    // ECS
    registry _registry;
    uint32_t _nextNetworkId;

    // Map player ID to entity
    std::unordered_map<uint8_t, Entity> _playerEntities;

    // Game loop control
    std::atomic<bool> _gameRunning;
    std::thread _gameThread;
    
    // Tick rate (60 updates per second)
    static constexpr int TICK_RATE = 60;
    static constexpr float TICK_INTERVAL = 1.0f / TICK_RATE;
};
