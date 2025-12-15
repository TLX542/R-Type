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
#include <filesystem>
#include <cmath>

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
        std::cerr << "Warning: Failed to load background image from 'assets/starfield.jpeg'" << std::endl;
        std::cerr << "Current working directory: " << std::filesystem::current_path() << std::endl;
        std::cerr << "Generating procedural starfield as fallback..." << std::endl;
        
        // Generate procedural starfield
        Image starfieldImg = GenImageColor(800, 600, BLACK);
        for (int i = 0; i < 300; i++) {
            int x = GetRandomValue(0, 799);
            int y = GetRandomValue(0, 599);
            // Use white or near-white colors
            Color starColor = {
                static_cast<unsigned char>(GetRandomValue(200, 255)),
                static_cast<unsigned char>(GetRandomValue(200, 255)),
                static_cast<unsigned char>(GetRandomValue(200, 255)),
                255
            };
            ImageDrawPixel(&starfieldImg, x, y, starColor);
        }
        background = LoadTextureFromImage(starfieldImg);
        UnloadImage(starfieldImg);
        
        if (background.id == 0) {
            std::cerr << "Error: Failed to generate fallback starfield texture" << std::endl;
        } else {
            std::cout << "Successfully generated procedural starfield (" 
                      << background.width << "x" << background.height << ")" << std::endl;
        }
    } else {
        std::cout << "Successfully loaded starfield texture (" 
                  << background.width << "x" << background.height << ")" << std::endl;
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
        scrollX += scrollSpeed;
        
        // Render
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw scrolling background with texture-space wrapping
        if (background.id != 0) {
            float texOffset = fmod(scrollX, (float)background.width);
            float windowWidth = 800.0f;
            float windowHeight = 600.0f;
            
            // First segment: from texOffset to end of texture
            float firstSegmentWidth = background.width - texOffset;
            float firstScaledWidth = (firstSegmentWidth / background.width) * windowWidth;
            
            Rectangle source1 = {texOffset, 0, firstSegmentWidth, (float)background.height};
            Rectangle dest1 = {0, 0, firstScaledWidth, windowHeight};
            Vector2 origin = {0, 0};
            DrawTexturePro(background, source1, dest1, origin, 0.0f, WHITE);
            
            // Second segment: from start of texture to fill remaining window
            float remainingWidth = windowWidth - firstScaledWidth;
            if (remainingWidth > 0) {
                float secondSegmentWidth = (remainingWidth / windowWidth) * background.width;
                Rectangle source2 = {0, 0, secondSegmentWidth, (float)background.height};
                Rectangle dest2 = {firstScaledWidth, 0, remainingWidth, windowHeight};
                DrawTexturePro(background, source2, dest2, origin, 0.0f, WHITE);
            }
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
