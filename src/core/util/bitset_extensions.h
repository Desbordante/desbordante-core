/* This file contains custom implementation of _Find_first and _Find_next gcc-specific methods
(which come from SGI extensions) of std::bitset for 64-bit bitsets.
These implementations are close to what is in SGI (and are competitive in terms of efficiency).
It shouldn't be so hard to adapt them for bitsets of any width -- see, for example,
https://cocode.se/c++/unsigned_split.html.
If you need _Find_first or _Find_next methods, consider using FindFirst and FindNext from this file.
FindFirst and FindNext are wrappers that use custom implementations if (and only if) gcc intrinsiscs
aren't availible. */

#pragma once

#include <bitset>
#include <memory>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace util {

namespace bitset_extensions {

static std::vector<unsigned long long> const kBytes{
        0x00'00'00'00'00'00'00'ff, 0x00'00'00'00'00'00'ff'00, 0x00'00'00'00'00'ff'00'00,
        0x00'00'00'00'ff'00'00'00, 0x00'00'00'ff'00'00'00'00, 0x00'00'ff'00'00'00'00'00,
        0x00'ff'00'00'00'00'00'00, 0xff'00'00'00'00'00'00'00};
static std::vector<unsigned char> const kFirstBits{0b11111110, 0b11111100, 0b11111000, 0b11110000,
                                                   0b11100000, 0b11000000, 0b10000000, 0b00000000};
constexpr static size_t kNumBytes = 8;
constexpr static size_t kWidth = 64;

#if (__cpp_lib_constexpr_vector == 201907L)
#define CONSTEXPR_IF_VECTOR_IS_CONSTEXPR constexpr
#else
#define CONSTEXPR_IF_VECTOR_IS_CONSTEXPR /* Ignore */
#endif

CONSTEXPR_IF_VECTOR_IS_CONSTEXPR unsigned char GetByte(unsigned long long val, size_t byte_num);

size_t FindFirstFixedWidth(std::bitset<kWidth> const&);

size_t FindNextFixedWidth(std::bitset<kWidth> const&, size_t pos);

template <typename Bitset>
concept HasFindFirst = requires(Bitset bs) { bs._Find_first(); };

template <typename Bitset>
concept HasFindNext = requires(Bitset bs) { bs._Find_next(0); };

}  // namespace bitset_extensions

/// @brief Call bs._Find_first if it's availible, use custom implementation otherwise
template <size_t S>
    requires bitset_extensions::HasFindFirst<std::bitset<S>>
inline size_t FindFirst(std::bitset<S> const& bs) noexcept {
    return bs._Find_first();
}

/// @brief Call bs._Find_first if it's availible, use custom implementation otherwise
template <size_t S>
inline size_t FindFirst(std::bitset<S> const& bs) noexcept {
    if constexpr (S == 64) {
        return bitset_extensions::FindFirstFixedWidth(bs);
    } else {
        // TODO(senichenkov): implement custom FindFirst for 256-bit (or custom width) bitsets
        boost::dynamic_bitset<> dbs(bs.to_string());
        auto result = dbs.find_first();
        return result <= S ? result : S;
    }
}

/// @brief Call bs._Find_next if it's availible, use custom implementation otherwise
template <size_t S>
    requires bitset_extensions::HasFindNext<std::bitset<S>>
inline size_t FindNext(std::bitset<S> const& bs, size_t pos) noexcept {
    return bs._Find_next(pos);
}

/// @brief Call bs._Find_next if it's availible, use custom implementation otherwise
template <size_t S>
inline size_t FindNext(std::bitset<S> const& bs, size_t pos) noexcept {
    if constexpr (S == 64) {
        return bitset_extensions::FindNextFixedWidth(bs, pos);
    } else {
        // TODO(senichenkov): implement custom FindNext for 256-bit (or custom width) bitsets
        boost::dynamic_bitset<> dbs(bs.to_string());
        auto result = dbs.find_next(pos);
        return result <= S ? result : S;
    }
}

/// @brief If _Find_next is availible, copy every set bit, else copy biset to dynamic_bitset
/// through string representation. Bitset is shifted 1 bit left.
template <size_t S>
    requires bitset_extensions::HasFindFirst<std::bitset<S>> &&
             bitset_extensions::HasFindNext<std::bitset<S>>
inline boost::dynamic_bitset<> CreateShiftedDynamicBitset(std::bitset<S> const& bs,
                                                          std::size_t size = S) noexcept {
    boost::dynamic_bitset<> dyn_bitset(size);
    for (size_t i = bs._Find_first(); i != S; i = bs._Find_next(i)) {
        if (i > 0) {
            dyn_bitset.set(i - 1);
        }
    }
    return dyn_bitset;
}

/// @brief If _Find_next is availible, copy every set bit, else copy biset to dynamic_bitset
/// through string representation. Bitset is shifted 1 bit left.
template <size_t S>
inline boost::dynamic_bitset<> CreateShiftedDynamicBitset(std::bitset<S> const& bs,
                                                          std::size_t size = S) noexcept {
    size_t start = S - size - 1;
    return boost::dynamic_bitset(bs.to_string(), start, size);
}

/// @brief Wrapper for std::bitset to iterate through set bits using temporary
/// boost::dynamic_bitset.
template <size_t S>
class BitsetIterator {
private:
    boost::dynamic_bitset<> bs_;
    size_t pos_;

public:
    BitsetIterator(std::bitset<S> const& bs) : bs_(bs.to_string()), pos_(bs_.find_first()) {
        if (pos_ == boost::dynamic_bitset<>::npos) {
            pos_ = bs_.size();
        }
    }

    size_t Pos() const noexcept {
        return pos_;
    }

    void Next() noexcept {
        pos_ = bs_.find_next(pos_);
        if (pos_ == boost::dynamic_bitset<>::npos) {
            pos_ = bs_.size();
        }
    }
};

/// @brief Wrapper for std::bitset to iterate through set bits using GCC intrinsics.
/// If reference to bitset is invalidated, behaviour is undefined!
template <size_t S>
    requires bitset_extensions::HasFindFirst<std::bitset<S>> &&
             bitset_extensions::HasFindNext<std::bitset<S>>
class BitsetIterator<S> {
private:
    std::bitset<S> const& bs_;
    size_t pos_;

public:
    BitsetIterator(std::bitset<S> const& bs) : bs_(bs), pos_(bs_._Find_first()) {}

    size_t Pos() const noexcept {
        return pos_;
    }

    void Next() noexcept {
        pos_ = bs_._Find_next(pos_);
    }
};

/// @brief Wrapper for 64-bit std::bitset to iterate through set bits using custom implementations.
/// If reference to bitset is invalidated, behaviour is undefined!
template <size_t S>
    requires(S == 64) && (!bitset_extensions::HasFindFirst<std::bitset<S>>) &&
            (!bitset_extensions::HasFindNext<std::bitset<S>>)
class BitsetIterator<S> {
private:
    std::bitset<64> const& bs_;
    size_t pos_;

public:
    BitsetIterator(std::bitset<64> const& bs) : bs_(bs), pos_(FindFirst(bs_)) {}

    size_t Pos() const noexcept {
        return pos_;
    }

    void Next() noexcept {
        pos_ = FindNext(bs_, pos_);
    }
};

}  // namespace util
