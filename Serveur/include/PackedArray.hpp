#pragma once
// PackedArray: stores only present entities and components densely.
// API is intentionally similar to sparse_array but optimized for density.
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <utility>

template <typename Component, typename EntityIdT = std::size_t>
class PackedArray {
public:
    using entity_type = EntityIdT;
    using component_type = Component;

    PackedArray() = default;

    // returns reference to component inserted (as stored in components_)
    Component& insert(entity_type ent, const Component& comp) {
        auto it = index_of(ent);
        if (it != npos) {
            components_[it] = comp;
            return components_[it];
        }
        index_map_[ent] = components_.size();
        entities_.push_back(ent);
        components_.push_back(comp);
        return components_.back();
    }

    Component& insert(entity_type ent, Component&& comp) {
        auto it = index_of(ent);
        if (it != npos) {
            components_[it] = std::move(comp);
            return components_[it];
        }
        index_map_[ent] = components_.size();
        entities_.push_back(ent);
        components_.push_back(std::move(comp));
        return components_.back();
    }

    template <typename... Args>
    Component& emplace(entity_type ent, Args&&... args) {
        auto it = index_of(ent);
        if (it != npos) {
            components_[it] = Component(std::forward<Args>(args)...);
            return components_[it];
        }
        index_map_[ent] = components_.size();
        entities_.push_back(ent);
        components_.emplace_back(std::forward<Args>(args)...);
        return components_.back();
    }

    void erase(entity_type ent) {
        auto it = index_of(ent);
        if (it == npos) return;
        // swap-remove to keep components dense
        size_t idx = it;
        size_t last = components_.size() - 1;
        if (idx != last) {
            components_[idx] = std::move(components_[last]);
            entities_[idx] = entities_[last];
            index_map_[entities_[idx]] = idx;
        }
        components_.pop_back();
        index_map_.erase(ent);
        entities_.pop_back();
    }

    // lookup index by entity, returns npos if not present
    size_t index_of(entity_type ent) const {
        auto it = index_map_.find(ent);
        if (it == index_map_.end()) return npos;
        return it->second;
    }

    bool contains(entity_type ent) const {
        return index_map_.find(ent) != index_map_.end();
    }

    // number of stored components (dense count)
    size_t count() const noexcept { return components_.size(); }

    // For compatibility: expose a max-like size for iterators; we'll let user manage
    // size() meaning number of active components (not max entity id) — zipper expects max index,
    // your hybrid wrapper will expose a suitable size() for zipper compatibility.
    size_t size() const noexcept { return components_.size(); }

    // accessors to entities and components arrays for iteration
    const std::vector<entity_type>& entities() const noexcept { return entities_; }
    std::vector<entity_type>& entities() noexcept { return entities_; }

    const std::vector<Component>& components() const noexcept { return components_; }
    std::vector<Component>& components() noexcept { return components_; }

    static constexpr size_t npos = static_cast<size_t>(-1);

private:
    std::vector<entity_type> entities_;
    std::vector<Component> components_;
    std::unordered_map<entity_type, size_t> index_map_;
};