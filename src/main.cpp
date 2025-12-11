#include "../include/GameServer.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char **argv) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <tcp_port> <udp_port>" << std::endl;
            std::cerr << "Example: " << argv[0] << " 4242 4243" << std::endl;
            return 1;
        }

        short tcpPort;
        short udpPort;
        try {
            int tcp = std::stoi(argv[1]);
            int udp = std::stoi(argv[2]);
            if (tcp <= 0 || tcp > 65535 || udp <= 0 || udp > 65535) {
                std::cerr << "Error: Ports must be between 1 and 65535" << std::endl;
                return 1;
            }
            tcpPort = static_cast<short>(tcp);
            udpPort = static_cast<short>(udp);
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid port number" << std::endl;
            return 1;
        }

        asio::io_context io_context;
        GameServer server(io_context, tcpPort, udpPort);

        std::cout << "R-Type Game Server is running..." << std::endl;
        std::cout << "Waiting for clients to connect..." << std::endl;

        // Start game loop in background thread
        server.startGameLoop();

        // Run network I/O
        server.run();

    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
