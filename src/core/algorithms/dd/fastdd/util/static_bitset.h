#pragma once

#include <cassert>
#include <cstddef>
#include <utility>

#include <boost/dynamic_bitset.hpp>

#include "core/model/types/bitset.h"

namespace algos::dd {

template <std::size_t N = 128>
class StaticBitset {
private:
    std::size_t size_;

    model::bitset_impl::BitsetImpl<N> static_bitset_;

public:
    static std::size_t const npos = N;

    StaticBitset() = default;

    explicit StaticBitset(std::size_t size) : size_(size), static_bitset_() {}

    StaticBitset(boost::dynamic_bitset<> const& bs) : StaticBitset(bs.size()) {
        for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            static_bitset_.set(index);
        }
    }

    bool operator[](std::size_t index) const {
        assert(index < size_);
        return static_bitset_[index];
    }

    StaticBitset& set() noexcept {
        static_bitset_.set();
        static_bitset_ >>= (N - size_);
        return *this;
    }

    StaticBitset& flip() noexcept {
        static_bitset_.flip();
        static_bitset_ <<= (N - size_);
        static_bitset_ >>= (N - size_);
        return *this;
    }

    StaticBitset& set(std::size_t index, bool value = true) {
        assert(index < size_);
        static_bitset_.set(index, value);
        return *this;
    }

    StaticBitset& reset(std::size_t index) {
        assert(index < size_);
        static_bitset_.reset(index);
        return *this;
    }

    bool none() const noexcept {
        return static_bitset_.none();
    }

    std::size_t size() const noexcept {
        return size_;
    }

    std::size_t count() const noexcept {
        return static_bitset_.count();
    }

    bool operator==(StaticBitset const& other) const = default;

    template <std::size_t Size>
    friend bool operator<(StaticBitset<Size> const& left, StaticBitset<Size> const& right);

    std::size_t find_first() const noexcept {
        return static_bitset_._Find_first();
    }

    std::size_t find_next(std::size_t index) const noexcept {
        return static_bitset_._Find_next(index);
    }

    StaticBitset& operator&=(StaticBitset const& other) noexcept {
        static_bitset_ &= other.static_bitset_;
        return *this;
    }

    StaticBitset& operator|=(StaticBitset const& other) noexcept {
        static_bitset_ |= other.static_bitset_;
        return *this;
    }

    StaticBitset& operator-=(StaticBitset const& other) noexcept {
        static_bitset_ &= ~other.static_bitset_;
        return *this;
    }

    bool is_subset_of(StaticBitset const& other) const noexcept {
        return (static_bitset_ & ~other.static_bitset_).none();
    }
};

template <std::size_t N>
inline StaticBitset<N> operator&(StaticBitset<N> const& left, StaticBitset<N> const& right) {
    StaticBitset<N> result(left);
    result &= right;
    return result;
}

template <std::size_t N>
inline StaticBitset<N> operator-(StaticBitset<N> const& left, StaticBitset<N> const& right) {
    StaticBitset<N> result(left);
    result -= right;
    return result;
}

template <std::size_t Size>
inline bool operator<(StaticBitset<Size> const& left, StaticBitset<Size> const& right) {
    assert(left_.size() == right.size());

    return left.static_bitset_ < right.static_bitset_;
}

}  // namespace algos::dd
