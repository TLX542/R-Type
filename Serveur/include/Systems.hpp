#pragma once

#include "Registry.hpp"
#include "Components.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>

// position_system: add velocity to position for entities that have both
inline void position_system(registry & r) {
    auto * positions = r.get_components_if<Position>();
    auto * velocities = r.get_components_if<Velocity>();
    if (!positions || !velocities) return;

    const std::size_t limit = std::min(positions->size(), velocities->size());
    for (std::size_t i = 0; i < limit; ++i) {
        auto &pos_opt = (*positions)[i];
        auto &vel_opt = (*velocities)[i];
        if (pos_opt && vel_opt) {
            Position & p = pos_opt.value();
            const Velocity & v = vel_opt.value();
            p.x += v.vx;
            p.y += v.vy;
        }
    }
}

// control_system: for entities with Controllable + Velocity, set velocity from keyboard
inline void control_system(registry & r) {
    auto * controllables = r.get_components_if<Controllable>();
    auto * velocities = r.get_components_if<Velocity>();
    if (!controllables || !velocities) return;

    const std::size_t limit = std::min(controllables->size(), velocities->size());
    for (std::size_t i = 0; i < limit; ++i) {
        auto &ctrl_opt = (*controllables)[i];
        auto &vel_opt = (*velocities)[i];
        if (ctrl_opt && vel_opt) {
            const Controllable & ctrl = ctrl_opt.value();
            Velocity & vel = vel_opt.value();

            float vx = 0.0f;
            float vy = 0.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
                vx = -ctrl.speed;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                vx = ctrl.speed;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
                vy = -ctrl.speed;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                vy = ctrl.speed;
            }
            vel.vx = vx;
            vel.vy = vy;
        }
    }
}

// draw system factory: returns a lambda that captures the RenderWindow reference
inline std::function<void(registry&)> make_draw_system(sf::RenderWindow & window) {
    return [&window](registry & r) {
        auto * positions = r.get_components_if<Position>();
        auto * drawables = r.get_components_if<Drawable>();
        if (!positions || !drawables) return;

        const std::size_t limit = std::min(positions->size(), drawables->size());
        sf::RectangleShape shape;
        for (std::size_t i = 0; i < limit; ++i) {
            auto & pos_opt = (*positions)[i];
            auto & draw_opt = (*drawables)[i];
            if (pos_opt && draw_opt) {
                const Position & p = pos_opt.value();
                const Drawable & d = draw_opt.value();
                shape.setSize({ d.width, d.height });
                shape.setFillColor(sf::Color(d.color.r, d.color.g, d.color.b, d.color.a));
                shape.setPosition(p.x, p.y);
                window.draw(shape);
            }
        }
    };
}