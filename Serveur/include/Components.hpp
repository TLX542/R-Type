#pragma once

#include <cstdint>
#include <iostream>

// Basic POD components

struct Position {
    float x{0.0f};
    float y{0.0f};
    Position() = default;
    Position(float xx, float yy) : x(xx), y(yy) {}
    void print() const { std::cout << "Position(" << x << ", " << y << ")"; }
};

struct Velocity {
    float vx{0.0f};
    float vy{0.0f};
    Velocity() = default;
    Velocity(float a, float b) : vx(a), vy(b) {}
    void print() const { std::cout << "Velocity(" << vx << ", " << vy << ")"; }
};

struct Color {
    uint8_t r{255}, g{255}, b{255}, a{255};
    Color() = default;
    Color(uint8_t rr, uint8_t gg, uint8_t bb, uint8_t aa = 255) : r(rr), g(gg), b(bb), a(aa) {}
};

struct Drawable {
    float width{16.0f};
    float height{16.0f};
    Color color;
    Drawable() = default;
    Drawable(float w, float h, Color c = Color()) : width(w), height(h), color(c) {}
};

struct Controllable {
    // speed in pixels-per-second for framerate independence.
    // Default chosen so ~4 px/frame at 60 FPS -> 4 * 60 = 240 px/s
    float speed{240.0f};
    Controllable() = default;
    explicit Controllable(float s) : speed(s) {}
    // Get displacement (in pixels) for a given delta time in seconds
    float displacement(float dt) const { return speed * dt; }
};

// Network-specific components

struct NetworkId {
    uint32_t id{0};
    NetworkId() = default;
    explicit NetworkId(uint32_t i) : id(i) {}
};

struct PlayerOwner {
    uint8_t playerId{0};
    PlayerOwner() = default;
    explicit PlayerOwner(uint8_t pid) : playerId(pid) {}
};
