#pragma once

#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <asio.hpp>
#include "Game.hpp"
#include "Server.hpp"

// GameServer: Runs the authoritative game simulation and broadcasts state to clients
class GameServer {
public:
    GameServer(Server* networkServer, unsigned int tickRate = 60);
    ~GameServer();

    // Start the game loop in a separate thread
    void start();

    // Stop the game loop
    void stop();

    // Process player input from network
    void handlePlayerInput(uint8_t playerId, int8_t moveX, int8_t moveY, uint8_t buttons, uint32_t timestamp);

    // Add a player to the game
    void addPlayer(uint8_t playerId);

    // Remove a player from the game
    void removePlayer(uint8_t playerId);

    // Get the game instance
    game::Game& getGame() { return _game; }

private:
    void gameLoop();
    void broadcastSnapshot();
    void sendSnapshotToClient(const game::GameSnapshot& snapshot, std::shared_ptr<Session> session);

    game::Game _game;
    Server* _networkServer;
    unsigned int _tickRate;  // Ticks per second
    std::atomic<bool> _running;
    std::unique_ptr<std::thread> _gameThread;
    
    std::chrono::steady_clock::time_point _lastTickTime;
};
