#pragma once
// Updated zipper / indexed_zipper which uses container.get(index) to obtain
// a proxy optional-like value. This allows hybrid storages that return a
// optional_ref<T> by value to work seamlessly.
#include <tuple>
#include <utility>
#include <cstddef>
#include <type_traits>
#include <algorithm>

template <class... Containers>
class indexed_zipper_iterator {
    using containers_tuple = std::tuple<Containers*...>;
public:
    // value returned per container is decltype(std::declval<Containers>().get(0))
    template <class C>
    using get_result_t = decltype(std::declval<C>().get(std::size_t(0)));

    using produced_tuple = std::tuple<std::size_t, get_result_t<Containers>...>;
    using value_type = produced_tuple;
    using reference = value_type;
    using pointer = void;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
    using iterator_tuple = containers_tuple;

    indexed_zipper_iterator(iterator_tuple containers, std::size_t max, std::size_t idx = 0)
        : _containers(std::move(containers)), _max(max), _idx(idx)
    {
        if (_idx < _max && !all_set(std::index_sequence_for<Containers...>{})) ++(*this);
    }

    indexed_zipper_iterator(indexed_zipper_iterator const&) = default;

    indexed_zipper_iterator& operator++() {
        do { ++_idx; } while (_idx < _max && !all_set(std::index_sequence_for<Containers...>{}));
        return *this;
    }

    indexed_zipper_iterator operator++(int) {
        indexed_zipper_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    value_type operator*() const {
        return to_value(std::index_sequence_for<Containers...>{});
    }

    friend bool operator==(indexed_zipper_iterator const& a, indexed_zipper_iterator const& b) {
        return a._idx == b._idx && a._max == b._max && a._containers == b._containers;
    }
    friend bool operator!=(indexed_zipper_iterator const& a, indexed_zipper_iterator const& b) {
        return !(a == b);
    }

private:
    template <std::size_t... Is>
    bool all_set(std::index_sequence<Is...>) const {
        return (( _idx < std::get<Is>(_containers)->size() ) && ... )
            && (( static_cast<bool>( std::get<Is>(_containers)->get(_idx) ) ) && ...);
    }

    template <std::size_t... Is>
    value_type to_value(std::index_sequence<Is...>) const {
        return std::tuple<std::size_t, get_result_t<Containers>...>(_idx, (std::get<Is>(_containers)->get(_idx))...);
    }

    iterator_tuple _containers;
    std::size_t _max;
    std::size_t _idx;
};

template <class... Containers>
class indexed_zipper {
public:
    using iterator = indexed_zipper_iterator<Containers...>;
    using iterator_tuple = typename iterator::iterator_tuple;

    explicit indexed_zipper(Containers&... cs)
        : _containers(std::addressof(cs)...), _size(_compute_size(cs...))
    {}

    iterator begin() { return iterator(_containers, _size, 0); }
    iterator end()   { return iterator(_containers, _size, _size); }

private:
    static std::size_t _compute_size(Containers&... cs) {
        std::size_t sizes[] = { cs.size()... };
        return *std::max_element(std::begin(sizes), std::end(sizes));
    }

    std::tuple<Containers*...> _containers;
    std::size_t _size;
};

template <class... Containers>
indexed_zipper<Containers...> make_indexed_zipper(Containers&... cs) {
    return indexed_zipper<Containers...>(cs...);
}