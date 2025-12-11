#include "../include/GameClient.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>

GameClient::GameClient(const std::string& host, short tcpPort)
    : _tcpSocket(_ioContext),
      _udpSocket(_ioContext),
      _host(host),
      _tcpPort(tcpPort),
      _udpPort(0),
      _playerId(0),
      _sessionToken(0),
      _connected(false) {
}

GameClient::~GameClient() {
    if (_tcpSocket.is_open()) {
        _tcpSocket.close();
    }
    if (_udpSocket.is_open()) {
        _udpSocket.close();
    }
}

bool GameClient::connect(const std::string& username) {
    try {
        // Connect TCP
        std::cout << "[Client] Connecting to " << _host << ":" << _tcpPort << "..." << std::endl;
        asio::ip::tcp::resolver resolver(_ioContext);
        auto endpoints = resolver.resolve(_host, std::to_string(_tcpPort));
        asio::connect(_tcpSocket, endpoints);
        std::cout << "[Client] TCP connected" << std::endl;

        // Send CONNECT message
        TCPProtocol::Message connectMsg;
        connectMsg.type = TCPProtocol::CONNECT;
        connectMsg.params["username"] = username;
        connectMsg.params["version"] = "1.0";
        
        std::string request = connectMsg.serialize();
        asio::write(_tcpSocket, asio::buffer(request));

        // Receive CONNECT_OK
        char buffer[1024];
        size_t length = _tcpSocket.read_some(asio::buffer(buffer));
        std::string response(buffer, length);

        TCPProtocol::Message responseMsg = TCPProtocol::Message::parse(response);
        if (responseMsg.type != TCPProtocol::CONNECT_OK) {
            std::cerr << "[Client] Connection failed: " << responseMsg.type << std::endl;
            return false;
        }

        // Extract connection info
        _playerId = std::stoi(responseMsg.params["id"]);
        std::stringstream ss;
        ss << std::hex << responseMsg.params["token"];
        ss >> _sessionToken;
        _udpPort = std::stoi(responseMsg.params["udp_port"]);

        std::cout << "[Client] Authenticated!" << std::endl;
        std::cout << "         Player ID: " << (int)_playerId << std::endl;
        std::cout << "         Token: 0x" << std::hex << _sessionToken << std::dec << std::endl;
        std::cout << "         UDP Port: " << _udpPort << std::endl;

        // Setup UDP
        _udpSocket.open(asio::ip::udp::v4());
        asio::ip::udp::resolver udpResolver(_ioContext);
        auto udpEndpoints = udpResolver.resolve(asio::ip::udp::v4(), _host, std::to_string(_udpPort));
        _udpEndpoint = *udpEndpoints.begin();

        // Send initial UDP packet to establish connection
        sendInput(0, 0, 0);

        _connected = true;
        std::cout << "[Client] Ready to play!" << std::endl;
        return true;

    } catch (std::exception& e) {
        std::cerr << "[Client] Connection error: " << e.what() << std::endl;
        return false;
    }
}

void GameClient::sendInput(int8_t moveX, int8_t moveY, uint8_t buttons) {
    if (!_connected) {
        return;
    }

    // Build packet
    PacketHeader header;
    header.type = PLAYER_INPUT;
    header.payloadSize = sizeof(PlayerInputPayload);
    header.sessionToken = _sessionToken;

    PlayerInputPayload payload;
    payload.timestamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    payload.playerId = _playerId;
    payload.buttons = buttons;
    payload.moveX = moveX;
    payload.moveY = moveY;

    // Send packet
    std::vector<char> packet(sizeof(PacketHeader) + sizeof(PlayerInputPayload));
    std::memcpy(packet.data(), &header, sizeof(PacketHeader));
    std::memcpy(packet.data() + sizeof(PacketHeader), &payload, sizeof(PlayerInputPayload));

    _udpSocket.send_to(asio::buffer(packet), _udpEndpoint);
}

void GameClient::update() {
    if (!_connected) {
        return;
    }

    // Process all available UDP packets (non-blocking)
    while (true) {
        std::error_code ec;
        size_t available = _udpSocket.available(ec);
        
        if (ec || available == 0) {
            break;
        }

        size_t length = _udpSocket.receive_from(
            asio::buffer(_udpBuffer, sizeof(_udpBuffer)),
            _udpSenderEndpoint,
            0,  // flags
            ec
        );

        if (ec || length < sizeof(PacketHeader)) {
            continue;
        }

        // Parse header
        PacketHeader header;
        std::memcpy(&header, _udpBuffer, sizeof(PacketHeader));

        // Validate packet
        if (!validatePacket(header, length)) {
            continue;
        }

        // Handle packet based on type
        switch (header.type) {
            case ENTITY_SPAWN:
                handleEntitySpawn(_udpBuffer);
                break;
            case ENTITY_UPDATE:
                handleEntityUpdate(_udpBuffer);
                break;
            case ENTITY_BATCH_UPDATE:
                handleEntityBatchUpdate(_udpBuffer);
                break;
            case ENTITY_DESTROY:
                handleEntityDestroy(_udpBuffer);
                break;
            default:
                break;
        }
    }
}

void GameClient::handleEntitySpawn(const char* data) {
    EntitySpawnPayload payload;
    std::memcpy(&payload, data + sizeof(PacketHeader), sizeof(EntitySpawnPayload));

    ClientEntity entity;
    entity.networkId = payload.networkId;
    entity.x = payload.posX;
    entity.y = payload.posY;
    entity.width = 48.0f;
    entity.height = 48.0f;
    entity.health = payload.health;

    // Use different colors based on entity type
    if (payload.entityType == PLAYER) {
        // Different color for each player
        uint8_t colors[][3] = {
            {200, 30, 30},   // Red
            {30, 200, 30},   // Green
            {30, 30, 200},   // Blue
            {200, 200, 30}   // Yellow
        };
        int colorIdx = (payload.ownerPlayer - 1) % 4;
        entity.r = colors[colorIdx][0];
        entity.g = colors[colorIdx][1];
        entity.b = colors[colorIdx][2];
    } else {
        entity.r = 255;
        entity.g = 255;
        entity.b = 255;
    }

    _entities[entity.networkId] = entity;
    
    std::cout << "[Client] Entity spawned: ID=" << entity.networkId 
              << " at (" << entity.x << ", " << entity.y << ")" << std::endl;
}

void GameClient::handleEntityUpdate(const char* data) {
    EntityUpdatePayload payload;
    std::memcpy(&payload, data + sizeof(PacketHeader), sizeof(EntityUpdatePayload));

    auto it = _entities.find(payload.networkId);
    if (it != _entities.end()) {
        it->second.x = payload.posX;
        it->second.y = payload.posY;
        it->second.health = payload.health;
    }
}

void GameClient::handleEntityBatchUpdate(const char* data) {
    uint8_t count;
    std::memcpy(&count, data + sizeof(PacketHeader), sizeof(uint8_t));

    const char* entryData = data + sizeof(PacketHeader) + sizeof(uint8_t);
    
    for (uint8_t i = 0; i < count; ++i) {
        EntityBatchEntry entry;
        std::memcpy(&entry, entryData + i * sizeof(EntityBatchEntry), sizeof(EntityBatchEntry));

        auto it = _entities.find(entry.networkId);
        if (it != _entities.end()) {
            it->second.x = entry.posX;
            it->second.y = entry.posY;
            it->second.health = entry.health;
        }
    }
}

void GameClient::handleEntityDestroy(const char* data) {
    EntityDestroyPayload payload;
    std::memcpy(&payload, data + sizeof(PacketHeader), sizeof(EntityDestroyPayload));

    auto it = _entities.find(payload.networkId);
    if (it != _entities.end()) {
        std::cout << "[Client] Entity destroyed: ID=" << payload.networkId << std::endl;
        _entities.erase(it);
    }
}
