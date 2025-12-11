#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <asio.hpp>
#include <map>
#include <cstring>

// Include protocol definitions
#include "../../Serveur/include/Protocol.hpp"

// Simple entity representation for rendering
struct RenderEntity {
    uint32_t networkId;
    float x, y;
    float width, height;
    sf::Color color;
};

class RTypeClient {
public:
    RTypeClient(const std::string& host, short tcpPort)
        : _host(host)
        , _tcpPort(tcpPort)
        , _connected(false)
        , _running(true)
        , _playerId(0)
        , _sessionToken(0)
        , _udpPort(0)
        , _ioContext()
        , _tcpSocket(_ioContext)
        , _udpSocket(_ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0))
    {
    }

    bool connect(const std::string& username) {
        try {
            // Resolve and connect TCP
            asio::ip::tcp::resolver resolver(_ioContext);
            auto endpoints = resolver.resolve(_host, std::to_string(_tcpPort));
            asio::connect(_tcpSocket, endpoints);

            std::cout << "[Client] Connected to server via TCP" << std::endl;

            // Send CONNECT message
            std::string connectMsg = "CONNECT username=" + username + " version=1.0\n";
            asio::write(_tcpSocket, asio::buffer(connectMsg));

            // Read response
            asio::streambuf response;
            asio::read_until(_tcpSocket, response, "\n");
            std::istream response_stream(&response);
            std::string response_line;
            std::getline(response_stream, response_line);

            // Parse response
            std::istringstream iss(response_line);
            std::string type;
            iss >> type;

            if (type == "CONNECT_OK") {
                // Parse parameters
                std::string param;
                while (iss >> param) {
                    size_t eq = param.find('=');
                    if (eq != std::string::npos) {
                        std::string key = param.substr(0, eq);
                        std::string value = param.substr(eq + 1);
                        if (key == "id") {
                            _playerId = std::stoi(value);
                        } else if (key == "token") {
                            _sessionToken = std::stoul(value, nullptr, 16);
                        } else if (key == "udp_port") {
                            _udpPort = std::stoi(value);
                        }
                    }
                }

                std::cout << "[Client] Connection successful!" << std::endl;
                std::cout << "[Client] Player ID: " << (int)_playerId << std::endl;
                std::cout << "[Client] Session Token: 0x" << std::hex << _sessionToken << std::dec << std::endl;
                std::cout << "[Client] UDP Port: " << _udpPort << std::endl;

                _connected = true;

                // Start network thread
                _networkThread = std::thread(&RTypeClient::networkLoop, this);

                // Initialize UDP connection with PING
                sendPing();

                return true;
            } else {
                std::cerr << "[Client] Connection failed: " << response_line << std::endl;
                return false;
            }

        } catch (std::exception& e) {
            std::cerr << "[Client] Connection error: " << e.what() << std::endl;
            return false;
        }
    }

    void run() {
        // Create SFML window
        sf::RenderWindow window(sf::VideoMode(800, 600), "R-Type Client");
        window.setFramerateLimit(60);

        // Main render loop
        while (window.isOpen() && _running) {
            // Event handling
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                    _running = false;
                }
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                    window.close();
                    _running = false;
                }
            }

            // Send input to server
            sendInput();

            // Render
            window.clear(sf::Color::Black);
            
            // Draw entities
            {
                std::lock_guard<std::mutex> lock(_entitiesMutex);
                sf::RectangleShape shape;
                for (const auto& [id, entity] : _entities) {
                    shape.setSize(sf::Vector2f(entity.width, entity.height));
                    shape.setFillColor(entity.color);
                    shape.setPosition(entity.x, entity.y);
                    window.draw(shape);
                }
            }

            window.display();
        }

        // Cleanup
        stop();
    }

    void stop() {
        _running = false;
        if (_networkThread.joinable()) {
            _networkThread.join();
        }
    }

private:
    void networkLoop() {
        std::cout << "[Client] Network loop started" << std::endl;
        
        // Start async UDP receive
        startUdpReceive();
        
        // Run io_context in this thread
        _ioContext.run();
    }

    void startUdpReceive() {
        _udpSocket.async_receive_from(
            asio::buffer(_udpBuffer, sizeof(_udpBuffer)),
            _udpEndpoint,
            [this](std::error_code ec, std::size_t length) {
                if (!ec && length > 0) {
                    handleUdpPacket(_udpBuffer, length);
                }
                if (_running) {
                    startUdpReceive();
                }
            }
        );
    }

    void handleUdpPacket(const char* data, size_t length) {
        if (length < sizeof(PacketHeader)) return;

        PacketHeader header;
        std::memcpy(&header, data, sizeof(PacketHeader));

        if (header.magic != PROTOCOL_MAGIC || header.version != PROTOCOL_VERSION) {
            return;
        }

        switch (header.type) {
            case ENTITY_BATCH_UPDATE: {
                if (length >= sizeof(PacketHeader) + sizeof(uint8_t)) {
                    uint8_t count;
                    std::memcpy(&count, data + sizeof(PacketHeader), sizeof(uint8_t));

                    size_t expectedSize = sizeof(PacketHeader) + sizeof(uint8_t) + count * sizeof(EntityBatchEntry);
                    if (length >= expectedSize) {
                        EntityBatchEntry entries[MAX_BATCH_ENTITIES];
                        std::memcpy(entries, data + sizeof(PacketHeader) + sizeof(uint8_t), 
                                   count * sizeof(EntityBatchEntry));

                        std::lock_guard<std::mutex> lock(_entitiesMutex);
                        for (uint8_t i = 0; i < count; ++i) {
                            uint32_t id = entries[i].networkId;
                            auto it = _entities.find(id);
                            if (it == _entities.end()) {
                                // New entity
                                RenderEntity entity;
                                entity.networkId = id;
                                entity.x = entries[i].posX;
                                entity.y = entries[i].posY;
                                entity.width = 32.0f;
                                entity.height = 32.0f;
                                entity.color = sf::Color(100, 150, 200);
                                _entities[id] = entity;
                            } else {
                                // Update existing
                                it->second.x = entries[i].posX;
                                it->second.y = entries[i].posY;
                            }
                        }
                    }
                }
                break;
            }
            case PONG: {
                // std::cout << "[Client] Received PONG" << std::endl;
                break;
            }
            default:
                break;
        }
    }

    void sendInput() {
        if (!_connected) return;

        // Read keyboard input
        int8_t moveX = 0, moveY = 0;
        uint8_t buttons = 0;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
            moveX = -1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            moveX = 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) {
            moveY = -1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            moveY = 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            buttons |= BTN_SHOOT;
        }

        // Only send if there's actual input
        if (moveX != 0 || moveY != 0 || buttons != 0) {
            PacketHeader header;
            header.type = PLAYER_INPUT;
            header.payloadSize = sizeof(PlayerInputPayload);
            header.sessionToken = _sessionToken;

            PlayerInputPayload payload;
            payload.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            payload.playerId = _playerId;
            payload.buttons = buttons;
            payload.moveX = moveX;
            payload.moveY = moveY;

            char buffer[sizeof(PacketHeader) + sizeof(PlayerInputPayload)];
            std::memcpy(buffer, &header, sizeof(PacketHeader));
            std::memcpy(buffer + sizeof(PacketHeader), &payload, sizeof(PlayerInputPayload));

            try {
                asio::ip::udp::resolver resolver(_ioContext);
                auto endpoints = resolver.resolve(asio::ip::udp::v4(), _host, std::to_string(_udpPort));
                _udpSocket.send_to(asio::buffer(buffer, sizeof(buffer)), *endpoints.begin());
            } catch (std::exception& e) {
                // Ignore send errors
            }
        }
    }

    void sendPing() {
        PacketHeader header;
        header.type = PING;
        header.payloadSize = 0;
        header.sessionToken = _sessionToken;

        try {
            asio::ip::udp::resolver resolver(_ioContext);
            auto endpoints = resolver.resolve(asio::ip::udp::v4(), _host, std::to_string(_udpPort));
            _udpSocket.send_to(asio::buffer(&header, sizeof(PacketHeader)), *endpoints.begin());
            std::cout << "[Client] Sent PING to server" << std::endl;
        } catch (std::exception& e) {
            std::cerr << "[Client] Error sending PING: " << e.what() << std::endl;
        }
    }

    std::string _host;
    short _tcpPort;
    bool _connected;
    std::atomic<bool> _running;
    uint8_t _playerId;
    uint32_t _sessionToken;
    short _udpPort;

    asio::io_context _ioContext;
    asio::ip::tcp::socket _tcpSocket;
    asio::ip::udp::socket _udpSocket;
    asio::ip::udp::endpoint _udpEndpoint;

    std::thread _networkThread;

    std::map<uint32_t, RenderEntity> _entities;
    std::mutex _entitiesMutex;

    char _udpBuffer[1024];
};

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <tcp_port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " localhost 4242" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    short port = std::atoi(argv[2]);

    RTypeClient client(host, port);
    
    if (!client.connect("Player")) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    client.run();

    return 0;
}
