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

/// @brief Wrapper for std::bitset to iterate through set bits.
class IBitsetIterator {
public:
    virtual ~IBitsetIterator() {}

    virtual size_t Pos() const noexcept = 0;
    virtual void Next() noexcept = 0;
};

namespace bitset_extensions {

static std::vector<unsigned long long> const kBytes{0xff,
                                                    0xff'00,
                                                    0xff'00'00,
                                                    0xff'00'00'00,
                                                    0xff'00'00'00'00,
                                                    0xff'00'00'00'00'00,
                                                    0xff'00'00'00'00'00'00,
                                                    0xff'00'00'00'00'00'00'00};
constexpr static size_t kNumBytes = 8;
constexpr static size_t kWidth = 64;

constexpr unsigned char GetByte(unsigned long long val, size_t byte_num);

size_t FindFirstFixedWidth(std::bitset<kWidth> const&);

size_t FindNextFixedWidth(std::bitset<kWidth> const&, size_t pos);

/// @brief Test if T has method _Find_next using SFINAE
template <typename T>
struct TestBitset {
private:
    typedef char yes[1];
    typedef char no[2];

    template <typename Tp>
    static yes& Test(decltype(&Tp::_Find_next));

    template <typename Tp>
    static no& Test(...);

public:
    static bool const kValue = sizeof(Test<T>(nullptr)) == sizeof(yes);
};

/// @brief Test if T has method _Find_next
template <typename T>
constexpr bool TestBitsetV = TestBitset<T>::kValue;

/// @brief Wrapper for std::bitset to iterate through set bits using temporary
/// boost::dynamic_bitset.
template <size_t S>
class DynamicBitsetIterator : public IBitsetIterator {
private:
    boost::dynamic_bitset<> bs_;
    size_t pos_;

public:
    DynamicBitsetIterator(std::bitset<S> const& bs) : bs_(bs.to_string()), pos_(bs_.find_first()) {
        if (pos_ > bs_.size()) {
            pos_ = bs_.size();
        }
    }

    ~DynamicBitsetIterator() override = default;

    size_t Pos() const noexcept override {
        return pos_;
    }

    void Next() noexcept override {
        pos_ = bs_.find_next(pos_);
        if (pos_ > bs_.size()) {
            pos_ = bs_.size();
        }
    }
};

/// @brief Wrapper for std::bitset to iterate through set bits using GCC intrinsics.
/// If reference to bitset is invalidated, behaviour is undefined!
template <size_t S>
class BitsetIterator : public IBitsetIterator {
private:
    std::bitset<S> const& bs_;
    size_t pos_;

public:
    BitsetIterator(std::bitset<S> const& bs) : bs_(bs), pos_(bs_._Find_first()) {}

    ~BitsetIterator() override = default;

    size_t Pos() const noexcept override {
        return pos_;
    }

    void Next() noexcept override {
        pos_ = bs_._Find_next(pos_);
    }
};

}  // namespace bitset_extensions

/// @brief Call bs._Find_first if it's availible, use custom implementation otherwise
template <size_t S, typename std::enable_if<bitset_extensions::TestBitsetV<std::bitset<S>>,
                                            bool>::type = true>
inline size_t FindFirst(std::bitset<S> const& bs) noexcept {
    return bs._Find_first();
}

/// @brief Call bs._Find_first if it's availible, use custom implementation otherwise
template <size_t S, typename = std::enable_if_t<!bitset_extensions::TestBitsetV<std::bitset<S>>>>
inline size_t FindFirst(std::bitset<S> const& bs) noexcept {
    return bitset_extensions::FindFirstFixedWidth(bs);
}

/// @brief Call bs._Find_next if it's availible, use custom implementation otherwise
template <size_t S, typename std::enable_if<bitset_extensions::TestBitsetV<std::bitset<S>>,
                                            bool>::type = true>
inline size_t FindNext(std::bitset<S> const& bs, size_t pos) noexcept {
    return bs._Find_next(pos);
}

/// @brief Call bs._Find_next if it's availible, use custom implementation otherwise
template <size_t S, typename = std::enable_if_t<!bitset_extensions::TestBitsetV<std::bitset<S>>>>
inline size_t FindNext(std::bitset<S> const& bs, size_t pos) noexcept {
    if constexpr (S == 64) {
        return bitset_extensions::FindNextFixedWidth(bs, pos);
    } else {
        // FIXME(senichenkov): implement custom FindNext for 256-bit (or custom width) bitsets
        boost::dynamic_bitset<> dbs(bs.to_string());
        auto result = dbs.find_next(pos);
        return result <= S ? result : S;
    }
}

/// @brief If _Find_next is availible, copy every set bit, else copy biset to dynamic_bitset
/// through string representation
template <size_t S, typename std::enable_if<bitset_extensions::TestBitset<std::bitset<S>>::value,
                                            bool>::type = true>
inline boost::dynamic_bitset<> CreateDynamicBitset(std::bitset<S> const& bs,
                                                   std::size_t size = S) noexcept {
    boost::dynamic_bitset<> dyn_bitset(size);
    for (size_t i = bs._Find_first(); i != S; i = bs._Find_next(i)) {
        dyn_bitset.set(i - 1);
    }
    return dyn_bitset;
}

/// @brief If _Find_next is availible, copy every set bit, else copy biset to dynamic_bitset
/// through string representation
template <size_t S,
          typename = std::enable_if_t<!bitset_extensions::TestBitset<std::bitset<S>>::value>>
inline boost::dynamic_bitset<> CreateDynamicBitset(std::bitset<S> const& bs,
                                                   [[maybe_unused]] std::size_t size = S) noexcept {
    return boost::dynamic_bitset(bs.to_string());
}

/// @brief If _Find_next is availible, create std::bitset set-bits-iterator, else
/// boost::dynamic_bitset set-bits-iterator
template <size_t S, typename std::enable_if<bitset_extensions::TestBitsetV<std::bitset<S>>,
                                            bool>::type = true>
inline std::unique_ptr<IBitsetIterator> MakeBitsetIterator(std::bitset<S> const& bs) {
    return std::make_unique<bitset_extensions::BitsetIterator<S>>(bs);
}

/// @brief If _Find_next is availible, create std::bitset set-bits-iterator, else
/// boost::dynamic_bitset set-bits-iterator
template <size_t S, typename = std::enable_if_t<!bitset_extensions::TestBitsetV<std::bitset<S>>>>
inline std::unique_ptr<IBitsetIterator> MakeBitsetIterator(std::bitset<S> const& bs) {
    return std::make_unique<bitset_extensions::DynamicBitsetIterator<S>>(bs);
}

}  // namespace util
