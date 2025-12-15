#pragma once
/*
** Instrumented Registry
*/

#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <memory>
#include <functional>
#include <vector>
#include <utility>
#include <stdexcept>
#include <type_traits>
#include <iostream>

#include "Entity.hpp"
#include "HybridArray.hpp"

class registry {
public:
    using entity_t = Entity;

    registry() = default;
    ~registry() = default;

    // Register storage for Component and install erase callback
    template <class Component>
    HybridArray<Component>& register_component() {
        const std::type_index key(typeid(Component));

        auto it = _component_map.find(key);
        if (it == _component_map.end()) {
            auto up = std::make_unique<ComponentStorage<Component>>();
            auto res = _component_map.emplace(key, std::move(up));
            if (!res.second) {
                std::cerr << "[registry] emplace failed for " << key.name() << "\n";
            } 
            // find iterator again
            it = _component_map.find(key);
            if (it == _component_map.end()) {
                std::cerr << "[registry] ERROR: cannot find just-emplaced component storage for " << key.name() << "\n";
                throw std::runtime_error("register_component failed to emplace storage");
            }

            // store erase function for this component type
            _erase_map.emplace(key, [](registry& reg, entity_t const& e) {
                reg.remove_component<Component>(e);
            });

            return static_cast<ComponentStorage<Component>*>(it->second.get())->data;
        }

        return static_cast<ComponentStorage<Component>*>(it->second.get())->data;
    }

    // Non-const getter: creates if missing
    template <class Component>
    HybridArray<Component>& get_components() {
        return register_component<Component>();
    }

    // Const getter: throws if missing
    template <class Component>
    const HybridArray<Component>& get_components() const {
        const std::type_index key(typeid(Component));
        auto it = _component_map.find(key);
        if (it == _component_map.end()) {
            throw std::out_of_range("registry::get_components: component not registered");
        }
        return static_cast<ComponentStorage<Component>*>(it->second.get())->data;
    }

    // Non-creating pointer getter
    template <class Component>
    HybridArray<Component>* get_components_if() {
        const std::type_index key(typeid(Component));
        auto it = _component_map.find(key);
        if (it == _component_map.end()) return nullptr;
        return &static_cast<ComponentStorage<Component>*>(it->second.get())->data;
    }

    template <class Component>
    const HybridArray<Component>* get_components_if() const {
        const std::type_index key(typeid(Component));
        auto it = _component_map.find(key);
        if (it == _component_map.end()) return nullptr;
        return &static_cast<ComponentStorage<Component>*>(it->second.get())->data;
    }

    // Entities
    entity_t spawn_entity() {
        if (!_free_ids.empty()) {
            size_t id = _free_ids.back();
            _free_ids.pop_back();
            ++_alive_count;
            return entity_t(id);
        }
        size_t id = _next_id++;
        ++_alive_count;
        return entity_t(id);
    }

    entity_t entity_from_index(std::size_t idx) const {
        return entity_t(idx);
    }

    void kill_entity(entity_t const& e) {
        for (auto &kv : _erase_map) {
            if (kv.second) kv.second(*this, e);
        }
        _free_ids.push_back(static_cast<size_t>(e));
        if (_alive_count > 0) --_alive_count;
    }

    // add_component: returns a reference to the stored Component
    template <typename Component>
    Component& add_component(entity_t const& to, Component&& c) {
        // std::cerr << "[registry] add_component<" << typeid(Component).name() << ">() for entity " << static_cast<size_t>(to) << "\n";
        auto& storage = register_component<Component>();
        // std::cerr << "[registry] got storage reference at " << static_cast<void*>(&storage) << "\n";
        return storage.insert_at(static_cast<std::size_t>(to), std::forward<Component>(c));
    }

    // emplace component
    template <typename Component, typename ... Params>
    Component& emplace_component(entity_t const& to, Params&&... p) {
        auto& storage = register_component<Component>();
        return storage.emplace_at(static_cast<std::size_t>(to), std::forward<Params>(p)...);
    }

    // remove component if storage exists
    template <typename Component>
    void remove_component(entity_t const& from) {
        const std::type_index key(typeid(Component));
        auto it = _component_map.find(key);
        if (it == _component_map.end()) return;
        auto *storage = static_cast<ComponentStorage<Component>*>(it->second.get());
        storage->data.erase(static_cast<std::size_t>(from));
    }

    template <typename Component>
    bool has_component_storage() const {
        return _component_map.find(std::type_index(typeid(Component))) != _component_map.end();
    }

    // Systems support unchanged...
    template <class... Components, typename Function>
    void add_system(Function&& f) {
        using FunType = std::decay_t<Function>;
        FunType fn(std::forward<Function>(f));
        _systems.emplace_back([fn = std::move(fn)](registry& r) mutable {
            fn(r, r.template get_components<Components>()...);
        });
    }

    template <class... Components, typename Function>
    void add_system(Function const& f) {
        using FunType = std::decay_t<Function>;
        FunType fn(f);
        _systems.emplace_back([fn = std::move(fn)](registry& r) mutable {
            fn(r, r.template get_components<Components>()...);
        });
    }

    void run_systems() {
        for (auto &s : _systems) {
            if (s) s(*this);
        }
    }

private:
    struct IComponentStorage { virtual ~IComponentStorage() = default; };

    template <typename Component>
    struct ComponentStorage : IComponentStorage {
        HybridArray<Component> data;
    };

    std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> _component_map;
    std::unordered_map<std::type_index, std::function<void(registry&, entity_t const&)>> _erase_map;
    std::vector<std::function<void(registry&)>> _systems;

    std::size_t _next_id{0};
    std::vector<std::size_t> _free_ids;
    std::size_t _alive_count{0};
};