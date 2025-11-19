#pragma once
// HybridArray - simplified, robust sparse-backed implementation.
//
// This implementation intentionally uses a sparse vector (std::vector<std::optional<T>>)
// as the underlying storage for now. It exposes the same API we used earlier so it can
// be swapped into Registry.hpp without other code changes. The previous attempt to
// automatically switch between sparse and packed modes caused a crash; we'll reintroduce
// a packed-mode / conversion later after getting a stable baseline.
//
// Public API:
//  - insert_at(entity, comp) -> Component&
//  - emplace_at(entity, args...) -> Component&
//  - erase(entity)
//  - get(entity) -> optional_ref-like proxy (here returns std::optional<Component> by value)
//  - has(entity) -> bool
//  - size() -> size_t (sparse capacity, i.e., max entity id + 1)
//  - convert_to_packed / convert_to_sparse are provided as no-ops for compatibility
//  - is_packed() returns false
//
// Notes:
//  - This is intentionally conservative: insert_at and emplace_at always ensure the
//    vector has sufficient capacity before indexing, so no undefined behavior from
//    out-of-range operator[] occurs.
//  - For mutation in systems, prefer using operator[] (which returns std::optional<T>&)
//    or the get_ref() helper if you need a reference. The registry uses insert_at/emplace_at
//    and systems use make_indexed_zipper(...), which calls container.get(idx) — we keep
//    get() returning std::optional<T> by value for compatibility with zipper; if you want
//    proxies (optional_ref) we can change that later.
//
// This header aims to be a drop-in safe replacement for the hybrid header you had
// while preserving the rest of the codebase.
#include <vector>
#include <optional>
#include <cstddef>
#include <algorithm>
#include <utility>

template <typename Component, typename EntityIdT = std::size_t>
class HybridArray {
public:
    using entity_type = EntityIdT;
    using component_type = Component;
    using value_type = std::optional<Component>;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;

    HybridArray(float switch_density = 0.25f)
        : _switch_density(switch_density)
    {
        // start in sparse mode (no packing)
        _mode_is_packed = false;
    }

    // insert (copy)
    Component& insert_at(entity_type id, const Component& comp) {
        ensure_sparse_capacity(id + 1);
        if (!_sparse[id].has_value()) ++_sparse_count;
        _sparse[id] = comp;
        return _sparse[id].value();
    }

    // insert (move)
    Component& insert_at(entity_type id, Component&& comp) {
        ensure_sparse_capacity(id + 1);
        if (!_sparse[id].has_value()) ++_sparse_count;
        _sparse[id] = std::move(comp);
        return _sparse[id].value();
    }

    // emplace
    template <typename... Args>
    Component& emplace_at(entity_type id, Args&&... args) {
        ensure_sparse_capacity(id + 1);
        if (!_sparse[id].has_value()) ++_sparse_count;
        _sparse[id].emplace(std::forward<Args>(args)...);
        return _sparse[id].value();
    }

    // erase: make hole (do not shrink)
    void erase(entity_type id) {
        if (id < _sparse.size() && _sparse[id].has_value()) {
            _sparse[id] = std::nullopt;
            --_sparse_count;
        }
    }

    // get: return a copy of the optional (compatible with zipper which takes the returned value)
    std::optional<Component> get(entity_type id) const {
        if (id >= _sparse.size()) return std::nullopt;
        return _sparse[id];
    }

    // get_ref: return reference to stored optional so callers can mutate in-place
    // ensures capacity if needed
    reference_type get_ref(entity_type id) {
        ensure_sparse_capacity(id + 1);
        return _sparse[id];
    }

    bool has(entity_type id) const {
        return id < _sparse.size() && _sparse[id].has_value();
    }

    // For zipper compatibility: size() returns sparse capacity (max entity id + 1)
    size_t size() const noexcept {
        return _sparse.size();
    }

    // Conversion stubs (no-op for now)
    void convert_to_packed() {
        // no-op in the simplified implementation
        _mode_is_packed = false;
    }

    void convert_to_sparse() {
        // already sparse
        _mode_is_packed = false;
    }

    bool is_packed() const noexcept { return _mode_is_packed; }

    // Expose underlying sparse container for iteration if needed
    const std::vector<value_type>& sparse_data() const noexcept { return _sparse; }
    std::vector<value_type>& sparse_data() noexcept { return _sparse; }

    static constexpr size_t npos = static_cast<size_t>(-1);

private:
    void ensure_sparse_capacity(size_t newcap) {
        if (_sparse.size() < newcap) _sparse.resize(newcap);
    }

    bool _mode_is_packed{false};
    std::vector<value_type> _sparse;
    size_t _sparse_count{0};
    float _switch_density{0.25f};
};