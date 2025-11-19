#include <SFML/Graphics.hpp>
#include <iostream>
#include <algorithm>

#include "Registry.hpp"
#include "Components.hpp"
#include "HybridArray.hpp" // storage type used by registry
// Note: we don't use make_indexed_zipper here because of value-copy semantics
// introduced earlier; systems below access component storage by-reference via
// HybridArray::get_ref() so writes mutate the real storage.

int main()
{
    // Create SFML window
    sf::RenderWindow window(sf::VideoMode(800, 600), "ECS - updated main (hybrid + registry systems)");
    window.setFramerateLimit(60);

    // Create registry and register component storages (HybridArray created internally)
    registry reg;
    reg.register_component<Position>();
    reg.register_component<Velocity>();
    reg.register_component<Drawable>();
    reg.register_component<Controllable>();

    // Spawn one movable entity (position, velocity, drawable, controllable)
    auto player = reg.spawn_entity();
    reg.add_component<Position>(player, Position{100.0f, 100.0f});
    reg.add_component<Velocity>(player, Velocity{0.0f, 0.0f});
    reg.add_component<Drawable>(player, Drawable{48.0f, 48.0f, Color{200, 30, 30}});
    reg.add_component<Controllable>(player, Controllable{4.0f}); // speed in pixels per frame

    // Spawn a few static drawable entities (position + drawable only)
    for (int i = 0; i < 5; ++i) {
        auto e = reg.spawn_entity();
        float x = 50.0f + i * 120.0f;
        float y = 350.0f;
        reg.add_component<Position>(e, Position{x, y});
        reg.add_component<Drawable>(e, Drawable{64.0f, 32.0f, Color{30, static_cast<uint8_t>(100 + i * 30), 200}});
    }

    // Register systems using registry.add_system<...>(...)
    // Each system gets references to the HybridArray storages so they can call
    // get_ref(i) to obtain std::optional<Component>& and mutate the underlying data.

    // 1) Control system: sets Velocity for entities that have Controllable + Velocity.
    reg.add_system<Controllable, Velocity>([](registry& /*r*/, HybridArray<Controllable>& ctrls, HybridArray<Velocity>& vels) {
        std::size_t limit = std::min(ctrls.size(), vels.size());
        for (std::size_t i = 0; i < limit; ++i) {
            auto &ctrl_opt = ctrls.get_ref(i); // reference to stored optional
            auto &vel_opt  = vels.get_ref(i);  // reference to stored optional
            if (!ctrl_opt || !vel_opt) continue;

            auto const & ctrl = ctrl_opt.value();
            auto & vel = vel_opt.value();

            float vx = 0.0f;
            float vy = 0.0f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
                vx = -ctrl.speed;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
                vx = ctrl.speed;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) {
                vy = -ctrl.speed;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                vy = ctrl.speed;
            }

            // write through reference -> modifies actual stored Velocity
            vel.vx = vx;
            vel.vy = vy;
        }
    });

    // 2) Position system: integrates Velocity into Position for entities that have both.
    reg.add_system<Position, Velocity>([](registry& /*r*/, HybridArray<Position>& pos, HybridArray<Velocity>& vel) {
        std::size_t limit = std::min(pos.size(), vel.size());
        for (std::size_t i = 0; i < limit; ++i) {
            auto &pos_opt = pos.get_ref(i);
            auto &vel_opt = vel.get_ref(i);
            if (!pos_opt || !vel_opt) continue;
            auto & p = pos_opt.value();
            auto const & v = vel_opt.value();
            p.x += v.vx;
            p.y += v.vy;
        }
    });

    // 3) Draw system: draws all entities that have Position + Drawable.
    // Capture the window by reference; stored callable will be invoked later.
    reg.add_system<Position, Drawable>([&window](registry& /*r*/, HybridArray<Position>& pos, HybridArray<Drawable>& draw) {
        sf::RectangleShape shape;
        std::size_t limit = std::min(pos.size(), draw.size());
        for (std::size_t i = 0; i < limit; ++i) {
            auto & pos_opt = pos.get_ref(i);
            auto & draw_opt = draw.get_ref(i);
            if (!pos_opt || !draw_opt) continue;
            auto const & p = pos_opt.value();
            auto const & d = draw_opt.value();
            shape.setSize(sf::Vector2f(d.width, d.height));
            shape.setFillColor(sf::Color(d.color.r, d.color.g, d.color.b, d.color.a));
            shape.setPosition(p.x, p.y);
            window.draw(shape);
        }
    });

    // Main loop
    while (window.isOpen()) {
        // Event handling
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) window.close();
        }

        // Run all systems in registration order: control -> position -> draw
        window.clear(sf::Color::Black);
        reg.run_systems();
        window.display();
    }

    return 0;
}