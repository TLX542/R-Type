#pragma once

#include <asio.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <random>
#include "Protocol.hpp"

// Informations d'un client connecté
struct ClientInfo {
    uint8_t playerId;                          // ID du joueur (1-4)
    uint32_t sessionToken;                     // Token d'authentification UDP
    std::string username;                      // Nom du joueur
    asio::ip::udp::endpoint udpEndpoint;       // Endpoint UDP du client
    bool udpInitialized;                       // Premier paquet UDP reçu ?

    ClientInfo() : playerId(0), sessionToken(0), udpInitialized(false) {}
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(asio::ip::tcp::socket socket, int id, class Server* server);

    void start();
    void send(const std::string& message);
    int getId() const { return _clientId; }
    ClientInfo& getClientInfo() { return _clientInfo; }
    const ClientInfo& getClientInfo() const { return _clientInfo; }

private:
    void doRead();
    void doWrite();
    void handleMessage(const std::string& message);

    asio::ip::tcp::socket _socket;
    int _clientId;
    ClientInfo _clientInfo;
    Server* _server;
    enum { max_length = 1024 };
    char _data[max_length];
    std::vector<std::string> _writeQueue;
};

class Server {
public:
    Server(asio::io_context& io_context, short tcpPort, short udpPort);
    virtual ~Server() = default;

    void run();
    uint32_t generateSessionToken();
    short getUdpPort() const { return _udpPort; }
    size_t getClientCount() const { return _sessions.size(); }
    void broadcastMessage(const std::string& message, int excludeClientId = -1);

    // Virtual methods for subclasses to override
    virtual void handlePlayerInput(uint8_t playerId, int8_t moveX, int8_t moveY, uint8_t buttons) {
        (void)playerId; (void)moveX; (void)moveY; (void)buttons;
    }
    virtual void onPlayerConnected(uint8_t playerId) { (void)playerId; }
    virtual void onPlayerDisconnected(uint8_t playerId) { (void)playerId; }

protected:
    void doAccept();
    void doReceiveUDP();
    void handleUDPPacket(const char* data, size_t length,
                         const asio::ip::udp::endpoint& senderEndpoint);

    asio::io_context& _ioContext;
    asio::ip::tcp::acceptor _acceptor;
    asio::ip::udp::socket _udpSocket;
    short _udpPort;
    std::vector<std::shared_ptr<Session>> _sessions;
    int _nextClientId;
    std::mt19937 _randomGenerator;

    // Buffer pour UDP
    enum { udp_buffer_size = 1024 };
    char _udpBuffer[udp_buffer_size];
    asio::ip::udp::endpoint _udpSenderEndpoint;
};
