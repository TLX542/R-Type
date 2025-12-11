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

        short tcpPort = std::atoi(argv[1]);
        short udpPort = std::atoi(argv[2]);

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
