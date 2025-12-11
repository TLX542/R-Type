#include "../include/GameClient.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <tcp_port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " localhost 4242" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    short tcpPort = std::atoi(argv[2]);

    // Ask for username
    std::string username;
    std::cout << "Enter your name: ";
    std::getline(std::cin, username);
    if (username.empty()) {
        username = "Player";
    }

    // Create client and connect
    GameClient client(host, tcpPort);
    if (!client.connect(username)) {
        std::cerr << "Failed to connect to server" << std::endl;
        return 1;
    }

    // Create SFML window
    sf::RenderWindow window(sf::VideoMode(800, 600), "R-Type Client - Player " + std::to_string(client.getPlayerId()));
    window.setFramerateLimit(60);

    std::cout << "\n=== CONTROLS ===" << std::endl;
    std::cout << "Arrow Keys: Move" << std::endl;
    std::cout << "Space: Shoot" << std::endl;
    std::cout << "ESC: Quit" << std::endl;
    std::cout << "================\n" << std::endl;

    // Main loop
    while (window.isOpen()) {
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                window.close();
            }
        }

        // Read keyboard input
        int8_t moveX = 0;
        int8_t moveY = 0;
        uint8_t buttons = 0;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            moveX = -1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            moveX = 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            moveY = -1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            moveY = 1;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            buttons |= BTN_SHOOT;
        }

        // Send input to server
        client.sendInput(moveX, moveY, buttons);

        // Update client (receive world state)
        client.update();

        // Render
        window.clear(sf::Color::Black);

        // Draw all entities
        sf::RectangleShape shape;
        for (const auto& [networkId, entity] : client.getEntities()) {
            shape.setSize(sf::Vector2f(entity.width, entity.height));
            shape.setFillColor(sf::Color(entity.r, entity.g, entity.b));
            shape.setPosition(entity.x, entity.y);
            window.draw(shape);
        }

        // Draw info text
        sf::Font font;
        // Note: In a real application, you'd load a font file
        // For now, we'll skip text rendering if font loading fails
        
        window.display();
    }

    return 0;
}
