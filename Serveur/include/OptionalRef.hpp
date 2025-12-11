#pragma once
// optional_ref: a tiny optional<T&>-like proxy.
// - behaves like std::optional<T> but for references: has_value(), value(), operator bool()
// - small and trivially copyable (holds pointer T*).

#include <memory>      // std::addressof
#include <optional>    // std::bad_optional_access
#include <utility>
#include <stdexcept>

template <typename T>
class optional_ref {
public:
    using value_type = T;

    optional_ref() noexcept : _ptr(nullptr) {}
    optional_ref(std::nullptr_t) noexcept : _ptr(nullptr) {}
    explicit optional_ref(T& ref) noexcept : _ptr(std::addressof(ref)) {}

    bool has_value() const noexcept { return _ptr != nullptr; }
    explicit operator bool() const noexcept { return has_value(); }

    T& value() & {
        if (!_ptr) throw std::bad_optional_access();
        return *_ptr;
    }
    const T& value() const & {
        if (!_ptr) throw std::bad_optional_access();
        return *_ptr;
    }

    // value_or: return *ptr or fallback
    template <typename U>
    T& value_or(U& fallback) {
        return _ptr ? *_ptr : fallback;
    }

    // reset / assign
    void reset() noexcept { _ptr = nullptr; }
    void assign(T& ref) noexcept { _ptr = std::addressof(ref); }

private:
    T* _ptr;
};