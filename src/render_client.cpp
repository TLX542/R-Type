#ifdef _WIN32
#define NOGDI             // Prevent GDI functions that conflict with raylib
#define NOUSER            // Prevent user functions that conflict with raylib
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used Windows headers
#define NOMINMAX          // Prevent Windows.h from defining min/max macros
#endif

#include "../include/GameClient.hpp"
#include <raylib.h>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <tcp_port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " localhost 4242" << std::endl;
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

    // Create Raylib window
    InitWindow(800, 600, ("R-Type Client - Player " + std::to_string(client.getPlayerId())).c_str());
    SetTargetFPS(60);

    // Load scrolling background
    Texture2D background = LoadTexture("assets/starfield.jpeg");
    if (background.id == 0) {
        std::cerr << "Warning: Failed to load background image" << std::endl;
    }
    float scrollX = 0.0f;
    const float scrollSpeed = 1.0f;  // pixels per frame

    std::cout << "\n=== CONTROLS ===" << std::endl;
    std::cout << "Arrow Keys: Move" << std::endl;
    std::cout << "Space: Shoot" << std::endl;
    std::cout << "ESC: Quit" << std::endl;
    std::cout << "================\n" << std::endl;

    // Frame counter for periodic logging
    int frameCount = 0;
    static const bool VERBOSE_LOGGING = false;

    // Shooting cooldown
    double lastShootTime = 0.0;
    const double shootCooldown = 0.5;  // 0.5 seconds between shots

    // Main loop
    std::cout << "[Render] Entering main loop" << std::endl;
    while (!WindowShouldClose()) {
        frameCount++;
        double currentTime = GetTime();

        // Read keyboard input
        int8_t moveX = 0;
        int8_t moveY = 0;
        uint8_t buttons = 0;

        if (IsKeyDown(KEY_LEFT)) {
            moveX = -1;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            moveX = 1;
        }
        if (IsKeyDown(KEY_UP)) {
            moveY = -1;
        }
        if (IsKeyDown(KEY_DOWN)) {
            moveY = 1;
        }
        if (IsKeyDown(KEY_SPACE)) {
            // Check if cooldown has elapsed
            if (currentTime - lastShootTime >= shootCooldown) {
                buttons |= BTN_SHOOT;
                lastShootTime = currentTime;
            }
        }

        // Send input to server
        client.sendInput(moveX, moveY, buttons);

        // Update client (receive world state)
        client.update();

        // Periodic logging (every 60 frames = ~1 second at 60 FPS)
        if (VERBOSE_LOGGING && frameCount % 60 == 0) {
            std::cout << "[Render] Frame " << frameCount
                      << ", entities: " << client.getEntities().size() << std::endl;
        }

        // Update scrolling background
        scrollX -= scrollSpeed;
        if (scrollX <= -800.0f) {  // Window width
            scrollX = 0.0f;
        }

        // Render
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw scrolling background (draw twice for seamless loop, stretched to window size)
        if (background.id != 0) {
            Rectangle source = {0, 0, (float)background.width, (float)background.height};
            Rectangle dest1 = {scrollX, 0, 800, 600};  // Window size
            Rectangle dest2 = {scrollX + 800, 0, 800, 600};  // Second copy for seamless scrolling
            Vector2 origin = {0, 0};

            DrawTexturePro(background, source, dest1, origin, 0.0f, WHITE);
            DrawTexturePro(background, source, dest2, origin, 0.0f, WHITE);
        }

        // Draw all entities
        int drawnCount = 0;
        for (const auto& [networkId, entity] : client.getEntities()) {
            // Draw entity rectangle
            Color entityColor = {entity.r, entity.g, entity.b, 255};
            DrawRectangle((int)entity.x, (int)entity.y, (int)entity.width, (int)entity.height, entityColor);

            // Draw username above entity if it has one
            if (!entity.username.empty()) {
                const char* text = entity.username.c_str();
                int textWidth = MeasureText(text, 14);
                DrawText(text,
                        (int)(entity.x + (entity.width - textWidth) / 2.0f),
                        (int)(entity.y - 20.0f),
                        14,
                        WHITE);
            }

            drawnCount++;

            // Log first few entities on frame 1 for debugging
            if (VERBOSE_LOGGING && frameCount == 1) {
                std::cout << "[Render] Drawing entity " << networkId
                          << " at (" << entity.x << ", " << entity.y << ")"
                          << " size (" << entity.width << "x" << entity.height << ")"
                          << " color (" << (int)entity.r << "," << (int)entity.g << "," << (int)entity.b << ")"
                          << std::endl;
            }
        }

        // Log first frame and when entities change
        if (frameCount == 1 || (frameCount < 120 && drawnCount > 0 && frameCount % 60 == 0)) {
            std::cout << "[Render] Frame " << frameCount << ": Drawing " << drawnCount << " entities" << std::endl;
        }

        EndDrawing();
    }

    // Cleanup
    UnloadTexture(background);
    CloseWindow();
    std::cout << "[Render] Exiting main loop" << std::endl;
    return 0;
}
