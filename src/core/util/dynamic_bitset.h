#pragma once

#include <cstddef>
#include <utility>
#include <variant>

#include <boost/dynamic_bitset.hpp>

#include "core/model/types/bitset.h"

namespace util {

// Stores first N bits in static bitset, the remainder is stored in dynamic bitset
#if 0
template <std::size_t N = 128>
class DynamicBitset {
private:
    std::size_t size_;
    model::Bitset<N> static_bitset_;
    boost::dynamic_bitset<> dynamic_bitset_;

public:
    static std::size_t const npos = boost::dynamic_bitset<>::npos;

    DynamicBitset() = default;

    DynamicBitset(std::size_t size)
        : size_(size), static_bitset_(), dynamic_bitset_(size > N ? size - N : 0) {}

    DynamicBitset(boost::dynamic_bitset<> const& bs) : DynamicBitset(bs.size()) {
        for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            if (index < N) {
                static_bitset_.set(index);
            } else {
                dynamic_bitset_.set(index - N);
            }
        }
    }

    bool operator[](std::size_t index) const {
        assert(index < size_);
        return index < N ? static_bitset_[index] : dynamic_bitset_[index - N];
    }

    void set(std::size_t index, bool value = true) {
        assert(index < size_);
        if (index < N) {
            static_bitset_.set(index, value);
        } else {
            dynamic_bitset_.set(index - N, value);
        }
    }

    bool none() const {
        return static_bitset_.none() && dynamic_bitset_.none();
    }

    std::size_t size() const noexcept {
        return size_;
    }

    std::size_t find_first() const {
        std::size_t first = static_bitset_._Find_first();
        if (first != N) {
            return first;
        }
        first = dynamic_bitset_.find_first();
        return first == npos ? first : first + N;
    }

    std::size_t find_next(std::size_t index) const {
        if (index < N) {
            std::size_t next = static_bitset_._Find_next(index);
            if (next != N) {
                return next;
            }
            next = dynamic_bitset_.find_first();
            return next == npos ? next : next + N;
        }
        std::size_t next = dynamic_bitset_.find_next(index - N);
        return next == npos ? next : next + N;
    }

    bool operator==(DynamicBitset const& other) const = default;

    bool is_subset_of(DynamicBitset const& other) const {
        return (static_bitset_ & ~other.static_bitset_).none() &&
               dynamic_bitset_.is_subset_of(other.dynamic_bitset_);
    }

    boost::dynamic_bitset<> to_boost_dynamic_bitset() const {
        boost::dynamic_bitset<> bitset(size_);
        for (std::size_t index = find_first(); index != npos; index = find_next(index)) {
            bitset.set(index);
        }

        return bitset;
    }
};
#endif

// All bits are stored either in static or in dynamic bitset using std::variant
#if 0
template <std::size_t N = 128>
class DynamicBitset {
private:
    using bitset_t = std::variant<model::Bitset<N>, boost::dynamic_bitset<>>;
    std::size_t size_;

    bitset_t bitset_;

public:
    static std::size_t const npos = boost::dynamic_bitset<>::npos;

    DynamicBitset() = default;

    DynamicBitset(std::size_t size)
        : size_(size),
          bitset_(size <= N ? bitset_t{model::Bitset<N>()}
                            : bitset_t{boost::dynamic_bitset<>(size)}) {}

    DynamicBitset(boost::dynamic_bitset<> const& bs) : DynamicBitset(bs.size()) {
        std::visit(
                [&bs](auto&& bitset) {
                    using T = std::decay_t<decltype(bitset)>;
                    if constexpr (std::is_same_v<T, model::Bitset<N>>) {
                        for (std::size_t index = bs.find_first();
                             index != boost::dynamic_bitset<>::npos; index = bs.find_next(index)) {
                            bitset.set(index);
                        }
                    } else {
                        bitset = bs;
                    }
                },
                bitset_);
    }

    bool operator[](std::size_t index) const {
        assert(index < size_);
        return std::visit([index](auto&& bitset) { return bitset[index]; }, bitset_);
    }

    void set(std::size_t index, bool value = true) {
        assert(index < size_);
        std::visit([index, value](auto&& bitset) { bitset.set(index, value); }, bitset_);
    }

    bool none() const {
        return std::visit([](auto&& bitset) { return bitset.none(); }, bitset_);
    }

    std::size_t size() const noexcept {
        return size_;
    }

    std::size_t find_first() const {
        return std::visit(
                [](auto&& bitset) {
                    using T = std::decay_t<decltype(bitset)>;
                    if constexpr (std::is_same_v<T, model::Bitset<N>>) {
                        std::size_t first_index = bitset._Find_first();
                        return first_index == N ? npos : first_index;
                    } else {
                        return bitset.find_first();
                    }
                },
                bitset_);
    }

    std::size_t find_next(std::size_t index) const {
        return std::visit(
                [index](auto&& bitset) {
                    using T = std::decay_t<decltype(bitset)>;
                    if constexpr (std::is_same_v<T, model::Bitset<N>>) {
                        std::size_t next_index = bitset._Find_next(index);
                        return next_index == N ? npos : next_index;
                    } else {
                        return bitset.find_next(index);
                    }
                },
                bitset_);
    }

    bool operator==(DynamicBitset const& other) const = default;

    bool is_subset_of(DynamicBitset const& other) const {
        return std::visit(
                [&other](auto&& bitset) {
                    using T = std::decay_t<decltype(bitset)>;
                    if constexpr (std::is_same_v<T, model::Bitset<N>>) {
                        return std::visit(
                                [&bitset](auto&& other_bitset) {
                                    using T = std::decay_t<decltype(other_bitset)>;
                                    if constexpr (std::is_same_v<T, model::Bitset<N>>) {
                                        return (bitset & ~other_bitset).none();
                                    } else {
                                        assert(false);
                                        return false;
                                    }
                                },
                                other.bitset_);
                    } else {
                        return std::visit(
                                [&bitset](auto&& other_bitset) {
                                    using T = std::decay_t<decltype(other_bitset)>;
                                    if constexpr (std::is_same_v<T, boost::dynamic_bitset<>>) {
                                        return bitset.is_subset_of(other_bitset);
                                    } else {
                                        assert(false);
                                        return false;
                                    }
                                },
                                other.bitset_);
                    }
                },
                bitset_);
    }

    boost::dynamic_bitset<> to_boost_dynamic_bitset() const {
        boost::dynamic_bitset<> bitset(size_);
        for (std::size_t index = find_first(); index != npos; index = find_next(index)) {
            bitset.set(index);
        }

        return bitset;
    }
};
#endif

// Store static and dynamic bitsets separately wasting some space
#if 0
template <std::size_t N = 128>
class DynamicBitset {
private:
    std::size_t size_;
    model::Bitset<N> static_bitset_;
    boost::dynamic_bitset<> dynamic_bitset_;

public:
    static std::size_t const npos = boost::dynamic_bitset<>::npos;

    DynamicBitset() = default;

    DynamicBitset(std::size_t size)
        : size_(size), static_bitset_(), dynamic_bitset_(size > N ? size : 0) {}

    DynamicBitset(boost::dynamic_bitset<> const& bs) : DynamicBitset(bs.size()) {
        if (size_ <= N) {
            for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
                 index = bs.find_next(index)) {
                static_bitset_.set(index);
            }
        } else {
            for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
                 index = bs.find_next(index)) {
                dynamic_bitset_.set(index);
            }
        }
    }

    bool operator[](std::size_t index) const {
        assert(index < size_);
        if (size_ <= N) [[likely]] {
            return static_bitset_[index];
        } else {
            return dynamic_bitset_[index];
        }
        // return size_ <= N ? static_bitset_[index] : dynamic_bitset_[index];
    }

    void set(std::size_t index, bool value = true) {
        assert(index < size_);
        if (size_ <= N) [[likely]] {
            static_bitset_.set(index, value);
        } else {
            dynamic_bitset_.set(index, value);
        }
    }

    bool none() const {
        return size_ <= N ? static_bitset_.none() : dynamic_bitset_.none();
    }

    std::size_t size() const noexcept {
        return size_;
    }

    std::size_t find_first() const {
        if (size_ <= N) [[likely]] {
            std::size_t const first_index = static_bitset_._Find_first();
            if (first_index == N) [[unlikely]] {
                return npos;
            } else {
                return first_index;
            }
            // return first_index == N ? npos : first_index;
        }
        return dynamic_bitset_.find_first();
    }

    std::size_t find_next(std::size_t index) const {
        if (size_ <= N) [[likely]] {
            std::size_t const next_index = static_bitset_._Find_next(index);
            if (next_index == N) [[unlikely]] {
                return npos;
            } else {
                return next_index;
            }
            // return next_index == N ? npos : next_index;
        }
        return dynamic_bitset_.find_next(index);
    }

    bool operator==(DynamicBitset const& other) const = default;

    bool is_subset_of(DynamicBitset const& other) const {
        return size_ <= N ? (static_bitset_ & ~other.static_bitset_).none()
                          : dynamic_bitset_.is_subset_of(other.dynamic_bitset_);
    }

    boost::dynamic_bitset<> to_boost_dynamic_bitset() const {
        boost::dynamic_bitset<> bitset(size_);
        for (std::size_t index = find_first(); index != npos; index = find_next(index)) {
            bitset.set(index);
        }

        return bitset;
    }
};
#endif

// Store npos as a field to simplify find_first() and find_next()
#if 0
template <std::size_t N = 128>
class DynamicBitset {
private:
    std::size_t size_;
    model::Bitset<N> static_bitset_;
    boost::dynamic_bitset<> dynamic_bitset_;

public:
    std::size_t npos;

    DynamicBitset() = default;

    DynamicBitset(std::size_t size)
        : size_(size),
          static_bitset_(),
          dynamic_bitset_(size > N ? size : 0),
          npos(size > N ? boost::dynamic_bitset<>::npos : N) {}

    DynamicBitset(boost::dynamic_bitset<> const& bs) : DynamicBitset(bs.size()) {
        if (size_ <= N) {
            for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
                 index = bs.find_next(index)) {
                static_bitset_.set(index);
            }
        } else {
            for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
                 index = bs.find_next(index)) {
                dynamic_bitset_.set(index);
            }
        }
    }

    bool operator[](std::size_t index) const {
        // assert(index < size_);
        if (size_ <= N) [[likely]] {
            return static_bitset_[index];
        } else {
            return dynamic_bitset_[index];
        }
        // return size_ <= N ? static_bitset_[index] : dynamic_bitset_[index];
    }

    void set(std::size_t index, bool value = true) {
        assert(index < size_);
        if (size_ <= N) [[likely]] {
            static_bitset_.set(index, value);
        } else {
            dynamic_bitset_.set(index, value);
        }
    }

    bool none() const {
        return size_ <= N ? static_bitset_.none() : dynamic_bitset_.none();
    }

    std::size_t size() const noexcept {
        return size_;
    }

    std::size_t find_first() const {
        if (size_ <= N) [[likely]] {
            return static_bitset_._Find_first();
            // return first_index == N ? npos : first_index;
        }
        return dynamic_bitset_.find_first();
    }

    std::size_t find_next(std::size_t index) const {
        if (size_ <= N) [[likely]] {
            return static_bitset_._Find_next(index);
            // return next_index == N ? npos : next_index;
        }
        return dynamic_bitset_.find_next(index);
    }

    bool operator==(DynamicBitset const& other) const = default;

    bool is_subset_of(DynamicBitset const& other) const {
        return size_ <= N ? (static_bitset_ & ~other.static_bitset_).none()
                          : dynamic_bitset_.is_subset_of(other.dynamic_bitset_);
    }

    boost::dynamic_bitset<> to_boost_dynamic_bitset() const {
        boost::dynamic_bitset<> bitset(size_);
        for (std::size_t index = find_first(); index != npos; index = find_next(index)) {
            bitset.set(index);
        }

        return bitset;
    }
};
#endif

// Imitating static bitset
#if 1
template <std::size_t N = 128>
class DynamicBitset {
private:
    std::size_t size_;

    model::Bitset<N> static_bitset_;

public:
    static std::size_t const npos = N;

    DynamicBitset() = default;

    DynamicBitset(std::size_t size) : size_(size), static_bitset_() {}

    DynamicBitset(boost::dynamic_bitset<> const& bs) : DynamicBitset(bs.size()) {
        for (std::size_t index = bs.find_first(); index != boost::dynamic_bitset<>::npos;
             index = bs.find_next(index)) {
            static_bitset_.set(index);
        }
    }

    bool operator[](std::size_t index) const {
        assert(index < size_);
        return static_bitset_[index];
    }

    void set(std::size_t index, bool value = true) {
        assert(index < size_);
        static_bitset_.set(index, value);
    }

    bool none() const {
        return static_bitset_.none();
    }

    std::size_t size() const noexcept {
        return size_;
    }

    std::size_t find_first() const {
        return static_bitset_._Find_first();
    }

    std::size_t find_next(std::size_t index) const {
        return static_bitset_._Find_next(index);
    }

    bool operator==(DynamicBitset const& other) const = default;

    bool is_subset_of(DynamicBitset const& other) const {
        return (static_bitset_ & ~other.static_bitset_).none();
    }

    boost::dynamic_bitset<> to_boost_dynamic_bitset() const {
        boost::dynamic_bitset<> bitset(size_);
        for (std::size_t index = find_first(); index != npos; index = find_next(index)) {
            bitset.set(index);
        }

        return bitset;
    }
};
#endif

}  // namespace util
