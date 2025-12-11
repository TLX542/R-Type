#pragma once

#include <cstddef>
class registry; // forward decl

class Entity {
public:
    operator size_t() const noexcept { return _id; }
    size_t getId() const noexcept { return _id; }

    bool operator==(const Entity& other) const noexcept { return _id == other._id; }
    bool operator!=(const Entity& other) const noexcept { return _id != other._id; }
    bool operator<(const Entity& other) const noexcept { return _id < other._id; }

private:
    explicit Entity(size_t id) : _id(id) {}
    size_t _id;
    friend class registry; // only registry can create entities
};