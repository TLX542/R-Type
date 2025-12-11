#include "../include/GameClient.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <tcp_port>" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    short tcpPort;
    try {
        int port = std::stoi(argv[2]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Error: Port must be between 1 and 65535" << std::endl;
            return 1;
        }
        tcpPort = static_cast<short>(port);
    } catch (const std::exception& e) {
        std::cerr << "Error: Invalid port number: " << argv[2] << std::endl;
        return 1;
    }

    // Create client and connect
    GameClient client(host, tcpPort);
    if (!client.connect("TestPlayer")) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    std::cout << "\n=== HEADLESS TEST CLIENT ===" << std::endl;
    std::cout << "Connected as player " << (int)client.getPlayerId() << std::endl;
    std::cout << "Running for 10 seconds..." << std::endl;

    // Run for 10 seconds
    for (int i = 0; i < 100; ++i) {
        // Send some input
        int8_t moveX = (i % 10 < 5) ? 1 : -1;
        int8_t moveY = 0;
        client.sendInput(moveX, moveY, 0);

        // Update (receive packets)
        client.update();

        // Log entity count every second
        if (i % 10 == 0) {
            std::cout << "[Test] Second " << (i/10) << ": " 
                      << client.getEntities().size() << " entities" << std::endl;
            
            // Log entity details
            for (const auto& [id, entity] : client.getEntities()) {
                std::cout << "  Entity " << id << ": pos(" 
                          << entity.x << ", " << entity.y << ") "
                          << "color(" << (int)entity.r << "," << (int)entity.g << "," << (int)entity.b << ")"
                          << std::endl;
            }
        }

        // Sleep for 100ms (10 updates per second)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n=== TEST COMPLETE ===" << std::endl;
    std::cout << "Final entity count: " << client.getEntities().size() << std::endl;
    
    if (client.getEntities().empty()) {
        std::cerr << "ERROR: No entities received!" << std::endl;
        return 1;
    }

    std::cout << "SUCCESS: Entities received and rendered!" << std::endl;
    return 0;
}
