#pragma once
#include <vector>
#include <optional>
#include <cstddef>
#include <utility>
#include <stdexcept>
#include <cassert>

/*
 * Minimal, safe sparse_array<T>
 *
 * Notes:
 * - insert_at / emplace_at ensure capacity by resizing before accessing operator[].
 * - operator[] remains unchecked for performance; use at() if you need bounds checking.
 * - get() returns a copy of the optional (unchanged semantics). If you need mutation by reference,
 *   use operator[] or get_ref() (added below) which returns std::optional<T>&.
 */

template <typename Component>
class sparse_array {
public:
    using value_type = std::optional<Component>;
    using reference_type = value_type&;
    using const_reference_type = const value_type&;
    using container_t = std::vector<value_type>;
    using size_type = typename container_t::size_type;
    using iterator = typename container_t::iterator;
    using const_iterator = typename container_t::const_iterator;

    sparse_array() = default;
    ~sparse_array() = default;
    sparse_array(const sparse_array&) = default;
    sparse_array(sparse_array&&) noexcept = default;
    sparse_array& operator=(const sparse_array&) = default;
    sparse_array& operator=(sparse_array&&) noexcept = default;

    // unchecked access (like std::vector::operator[])
    reference_type operator[](size_type idx) {
        // Note: caller must ensure idx < size()
        return _data[idx];
    }
    const_reference_type operator[](size_type idx) const {
        return _data[idx];
    }

    // checked access
    reference_type at(size_type idx) {
        if (idx >= _data.size()) throw std::out_of_range("sparse_array::at: out of range");
        return _data[idx];
    }
    const_reference_type at(size_type idx) const {
        if (idx >= _data.size()) throw std::out_of_range("sparse_array::at: out of range");
        return _data[idx];
    }

    // safe getter returning an optional copy (nullopt if out-of-range)
    value_type get(size_type idx) const {
        if (idx >= _data.size()) return std::nullopt;
        return _data[idx];
    }

    // get a reference to the stored optional so callers can mutate in-place
    reference_type get_ref(size_type idx) {
        if (idx >= _data.size()) {
            // return a reference only after resizing
            _data.resize(idx + 1);
        }
        return _data[idx];
    }

    iterator begin() { return _data.begin(); }
    const_iterator begin() const { return _data.begin(); }
    const_iterator cbegin() const { return _data.cbegin(); }
    iterator end() { return _data.end(); }
    const_iterator end() const { return _data.end(); }
    const_iterator cend() const { return _data.cend(); }

    size_type size() const { return _data.size(); }

    // insert (copy)
    reference_type insert_at(size_type pos, const Component& component) {
        if (pos >= _data.size()) _data.resize(pos + 1);
        _data[pos] = component;
        return _data[pos];
    }

    // insert (move)
    reference_type insert_at(size_type pos, Component&& component) {
        if (pos >= _data.size()) _data.resize(pos + 1);
        _data[pos] = std::move(component);
        return _data[pos];
    }

    // emplace
    template <class... Params>
    reference_type emplace_at(size_type pos, Params&&... args) {
        if (pos >= _data.size()) _data.resize(pos + 1);
        _data[pos].emplace(std::forward<Params>(args)...);
        return _data[pos];
    }

    // erase: make hole (do not shrink)
    void erase(size_type pos) {
        if (pos < _data.size()) _data[pos] = std::nullopt;
    }

private:
    container_t _data;
};