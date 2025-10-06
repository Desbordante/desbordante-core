#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>

#include <boost/dynamic_bitset.hpp>

namespace util {

template <class B>
concept SgiFindableBitset = requires(B const& b, std::size_t p) {
    { b._Find_first() } -> std::convertible_to<std::size_t>;
    { b._Find_next(p) } -> std::convertible_to<std::size_t>;
    { b.size() } -> std::convertible_to<std::size_t>;
};

template <SgiFindableBitset B>
std::size_t BitsetNpos(B const& b) noexcept {
    return b.size();
}

template <SgiFindableBitset B>
std::size_t BitsetFindFirst(B const& b) noexcept {
    return b._Find_first();
}

template <SgiFindableBitset B>
std::size_t BitsetFindNext(B const& b, std::size_t pos) noexcept {
    return b._Find_next(pos);
}

template <class Block, class Allocator>
auto BitsetNpos(boost::dynamic_bitset<Block, Allocator> const&) {
    return boost::dynamic_bitset<Block, Allocator>::npos;
}

template <class Block, class Allocator>
auto BitsetFindFirst(boost::dynamic_bitset<Block, Allocator> const& b) {
    return b.find_first();
}

template <class Block, class Allocator>
auto BitsetFindNext(boost::dynamic_bitset<Block, Allocator> const& b,
                    typename boost::dynamic_bitset<Block, Allocator>::size_type pos) {
    return b.find_next(pos);
}

template <class B>
concept BitsetFindable = requires(B const& b, std::size_t p) {
    { BitsetNpos(b) } -> std::convertible_to<std::size_t>;
    { BitsetFindFirst(b) } -> std::convertible_to<std::size_t>;
    { BitsetFindNext(b, p) } -> std::convertible_to<std::size_t>;
};

template <BitsetFindable Bitset>
class SetBitsView {
public:
    using IndexType = std::size_t;

    struct Iterator {
        using value_type = IndexType;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept = std::forward_iterator_tag;

        Iterator() = default;

        Iterator(Bitset const* bs, IndexType pos) noexcept : bs_(bs), pos_(pos) {}

        [[nodiscard]] value_type operator*() const noexcept {
            return pos_;
        }

        Iterator& operator++() noexcept {
            pos_ = BitsetFindNext(*bs_, pos_);
            return *this;
        }

        Iterator operator++(int) noexcept {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(Iterator const& a, Iterator const& b) noexcept {
            return a.bs_ == b.bs_ && a.pos_ == b.pos_;
        }

        friend bool operator!=(Iterator const& a, Iterator const& b) noexcept {
            return !(a == b);
        }

        [[nodiscard]] bool AtEnd() const noexcept {
            return pos_ == BitsetNpos(*bs_);
        }

    private:
        Bitset const* bs_ = nullptr;
        IndexType pos_ = 0;
    };

    struct Sentinel {
        friend bool operator==(Iterator const& it, Sentinel const&) noexcept {
            return it.AtEnd();
        }

        friend bool operator==(Sentinel const& s, Iterator const& it) noexcept {
            return it == s;
        }

        friend bool operator!=(Iterator const& it, Sentinel const& s) noexcept {
            return !(it == s);
        }

        friend bool operator!=(Sentinel const& s, Iterator const& it) noexcept {
            return !(s == it);
        }
    };

    explicit SetBitsView(Bitset const& bs) noexcept : bs_(&bs) {}

    [[nodiscard]] Iterator begin() const noexcept {
        return Iterator{bs_, BitsetFindFirst(*bs_)};
    }

    [[nodiscard]] static Sentinel end() noexcept {
        return {};
    }

private:
    Bitset const* bs_;
};

template <BitsetFindable Bitset>
[[nodiscard]] auto SetBits(Bitset const& bs) noexcept {
    return SetBitsView<Bitset>{bs};
}

}  // namespace util
