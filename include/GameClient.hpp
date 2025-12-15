#pragma once

#include "Protocol.hpp"
#include <asio.hpp>
#include <string>
#include <unordered_map>
#include <cstdint>

/**
 * @brief Represents a networked entity on the client side
 */
struct ClientEntity {
    uint32_t networkId;
    float x, y;
    float width, height;
    uint8_t health;
    uint8_t r, g, b;
    std::string username;

    ClientEntity() : networkId(0), x(0), y(0), width(48), height(48), health(100), r(255), g(255), b(255), username("") {}
};

/**
 * @brief Thin game client that connects to server, sends input, and receives world state
 * 
 * This client does NOT run game logic - it only:
 * - Connects to server via TCP
 * - Sends player input via UDP
 * - Receives world state updates via UDP
 * - Stores entity positions for rendering
 */
class GameClient {
public:
    GameClient(const std::string& host, short tcpPort);
    ~GameClient();

    // Connect to server and authenticate
    bool connect(const std::string& username);

    // Send player input to server
    void sendInput(int8_t moveX, int8_t moveY, uint8_t buttons);

    // Process incoming UDP packets (non-blocking)
    void update();

    // Get all entities for rendering
    const std::unordered_map<uint32_t, ClientEntity>& getEntities() const { return _entities; }

    // Getters
    uint8_t getPlayerId() const { return _playerId; }
    bool isConnected() const { return _connected; }

private:
    void receiveUDP();
    void handleEntitySpawn(const char* data);
    void handleEntityUpdate(const char* data);
    void handleEntityBatchUpdate(const char* data);
    void handleEntityDestroy(const char* data);

    asio::io_context _ioContext;
    asio::ip::tcp::socket _tcpSocket;
    asio::ip::udp::socket _udpSocket;
    asio::ip::udp::endpoint _udpEndpoint;

    std::string _host;
    short _tcpPort;
    short _udpPort;

    uint8_t _playerId;
    uint32_t _sessionToken;
    bool _connected;

    // Entities indexed by network ID
    std::unordered_map<uint32_t, ClientEntity> _entities;

    // UDP receive buffer
    char _udpBuffer[1024];
    asio::ip::udp::endpoint _udpSenderEndpoint;
};
