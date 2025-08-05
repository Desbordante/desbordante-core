/* This file contatins simplified implementation of std::bitset with SGI extensions
 * (_Find_first and _Find_next) implemented.
 *
 * It's C++20 "slice" (this won't meet the standard on later versions): all "version" macros were
 * expanded to improve readability.
 *
 * Actually, it's simplified copy of bitset file from libstdc++
 * (https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/std/bitset),
 * which has the following copyright:
 */

// =======================================================================

// Copyright (C) 2001-2025 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/*
 * Copyright (c) 1998
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

// =======================================================================

// Also, this file contains parts (hash functions) of bitset file from libc++
// (https://github.com/llvm/llvm-project/blob/main/libcxx/include/bitset),
// which has the following copyright:

// =======================================================================

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// =======================================================================

#pragma once

#include <bit>
#include <bitset>
#include <climits>
#include <cstring>
#include <istream>
#include <limits>
#include <locale>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <boost/container_hash/hash.hpp>
#include <easylogging++.h>

namespace model {

namespace bitset_impl {

static constexpr size_t kCharBit = CHAR_BIT;
static constexpr size_t kBitsPerWord = kCharBit * sizeof(unsigned long);
static constexpr size_t kBitsPerUll = kCharBit * sizeof(unsigned long long);

static constexpr size_t BitsetWords(size_t n) {
    return n / kBitsPerWord + (n % kBitsPerWord == 0 ? 0 : 1);
}

/// @brief Base class, general case.
template <size_t NumWords>
class BaseBitset {
public:
    using WordT = unsigned long;

    constexpr BaseBitset() noexcept : words_() {}

    constexpr BaseBitset(unsigned long long val) noexcept
        : words_{WordT(val)
#if LLONG_MAX > LONG_MAX
                         ,
                 WordT(val >> kBitsPerWord)
#endif
          } {
    }

    static constexpr size_t WhichWord(size_t pos) noexcept {
        return pos / kBitsPerWord;
    }

    static constexpr size_t WhichByte(size_t pos) noexcept {
        return (pos % kBitsPerWord) / kCharBit;
    }

    static constexpr size_t WhichBit(size_t pos) noexcept {
        return pos % kBitsPerWord;
    }

    static constexpr WordT MaskBit(size_t pos) noexcept {
        return (static_cast<WordT>(1)) << WhichBit(pos);
    }

    constexpr WordT& GetWord(size_t pos) noexcept {
        return words_[WhichWord(pos)];
    }

    constexpr WordT GetWord(size_t pos) const noexcept {
        return words_[WhichWord(pos)];
    }

    constexpr WordT const* GetData() const noexcept {
        return words_;
    }

    WordT& HiWord() noexcept {
        return words_[NumWords - 1];
    }

    WordT HiWord() const noexcept {
        return words_[NumWords - 1];
    }

    void DoAnd(BaseBitset<NumWords> const& x) noexcept {
        for (size_t i{0}; i < NumWords; ++i) {
            words_[i] &= x.words_[i];
        }
    }

    constexpr void DoOr(BaseBitset<NumWords> const& x) noexcept {
        for (size_t i{0}; i < NumWords; ++i) {
            words_[i] |= x.words_[i];
        }
    }

    constexpr void DoXor(BaseBitset<NumWords> const& x) noexcept {
        for (size_t i{0}; i < NumWords; ++i) {
            words_[i] ^= x.words_[i];
        }
    }

    constexpr void DoLeftShift(size_t shift) noexcept;
    constexpr void DoRightShift(size_t shift) noexcept;

    constexpr void DoFlip() noexcept {
        for (size_t i{0}; i < NumWords; ++i) {
            words_[i] = ~words_[i];
        }
    }

    constexpr void DoSet() noexcept {
        if (std::is_constant_evaluated()) {
            for (WordT& word : words_) {
                word = ~static_cast<WordT>(0);
            }
            return;
        }
        std::memset(words_, 0xFF, NumWords * sizeof(WordT));
    }

    constexpr void DoReset() noexcept {
        if (std::is_constant_evaluated()) {
            for (WordT& word : words_) {
                word = 0;
            }
            return;
        }
        std::memset(words_, 0, NumWords * sizeof(WordT));
    }

    constexpr bool IsEqual(BaseBitset<NumWords> const& x) const noexcept {
        if (std::is_constant_evaluated()) {
            for (size_t i{0}; i < NumWords; ++i) {
                if (words_[i] != x.words_[i]) {
                    return false;
                }
            }
            return true;
        }
        return !std::memcmp(words_, x.words_, NumWords * sizeof(WordT));
    }

    template <size_t NumBits>
    constexpr bool AreAll() const noexcept {
        for (size_t i{0}; i < NumWords - 1; ++i) {
            if (words_[i] != ~static_cast<WordT>(0)) {
                return false;
            }
        }
        return HiWord() == (~static_cast<WordT>(0) >> (NumWords * kBitsPerWord - NumBits));
    }

    constexpr bool IsAny() const noexcept {
        for (size_t i{0}; i < NumWords; ++i) {
            if (words_[i] != static_cast<WordT>(0)) {
                return true;
            }
        }
        return false;
    }

    constexpr size_t DoCount() const noexcept {
        size_t result = 0;
        for (size_t i{0}; i < NumWords; ++i) {
            result += std::popcount(words_[i]);
        }
        return result;
    }

    constexpr unsigned long DoToULong() const;
    constexpr unsigned long long DoToULLong() const;
    constexpr size_t DoFindFirst(size_t) const noexcept;
    constexpr size_t DoFindNext(size_t, size_t) const noexcept;
    // >>> From libc++
    size_t HashCode() const noexcept;
    // <<< From libc++

private:
    // 0 is the least significant word
    WordT words_[NumWords];
};

// Definitions of non-inline functions from BaseBitset.
template <size_t NumWords>
constexpr void BaseBitset<NumWords>::DoLeftShift(size_t shift) noexcept {
    if (shift != 0) [[likely]] {
        size_t const wshift = shift / kBitsPerWord;
        size_t const offset = shift % kBitsPerWord;

        if (offset == 0) {
            for (size_t n{NumWords - 1}; n >= wshift; --n) {
                words_[n] = words_[n - wshift];
            }
        } else {
            size_t const sub_offset = kBitsPerWord - offset;
            for (size_t n = NumWords - 1; n > wshift; --n) {
                words_[n] =
                        ((words_[n - wshift] << offset) | (words_[n - wshift - 1] >> sub_offset));
            }
            words_[wshift] = words_[0] << offset;
        }
        std::fill(std::next(words_, 0), std::next(words_, wshift), static_cast<WordT>(0));
    }
}

template <size_t NumWords>
constexpr void BaseBitset<NumWords>::DoRightShift(size_t shift) noexcept {
    if (shift != 0) [[likely]] {
        size_t const wshift = shift / kBitsPerWord;
        size_t offset = shift % kBitsPerWord;
        size_t limit = NumWords - wshift - 1;

        if (offset == 0) {
            for (size_t n{0}; n <= limit; ++n) {
                words_[n] = words_[n + wshift];
            }
        } else {
            size_t const sub_offset = (kBitsPerWord - offset);
            for (size_t n{0}; n < limit; ++n) {
                words_[n] =
                        ((words_[n + wshift] >> offset) | (words_[n + wshift + 1] << sub_offset));
                words_[limit] = words_[NumWords - 1] >> offset;
            }
        }
        std::fill(std::next(words_, limit + 1), std::next(words_, NumWords), static_cast<WordT>(0));
    }
}

template <size_t NumWords>
constexpr unsigned long BaseBitset<NumWords>::DoToULong() const {
    for (size_t i{1}; i < NumWords; ++i) {
        if (words_[i]) {
            throw std::overflow_error("BaseBitset::DoToULong");
        }
    }
    return words_[0];
}

template <size_t NumWords>
constexpr unsigned long long BaseBitset<NumWords>::DoToULLong() const {
#if ULLONG_MAX == ULONG_MAX
    return DoToULong();
#else
    for (size_t i{2}; i < NumWords; ++i) {
        if (words_[i]) {
            throw std::overflow_error("BaseBitset::DoToULLong");
        }
    }
    return words_[0] + (static_cast<unsigned long long>(words_[1]) << kBitsPerWord);
#endif
}

template <size_t NumWords>
constexpr size_t BaseBitset<NumWords>::DoFindFirst(size_t not_found) const noexcept {
    for (size_t i{0}; i < NumWords; ++i) {
        auto this_word = words_[i];
        if (this_word != static_cast<WordT>(0)) {
            return i * kBitsPerWord + std::countr_zero(this_word);
        }
    }
    return not_found;
}

// >>> From libc++
template <size_t NumWords>
size_t BaseBitset<NumWords>::HashCode() const noexcept {
    size_t hash = 0;
    for (size_t i{0}; i < NumWords; ++i) {
        hash ^= words_[i];
    }
    return hash;
}

// <<< From libc++

template <size_t NumWords>
constexpr size_t BaseBitset<NumWords>::DoFindNext(size_t prev, size_t not_found) const noexcept {
    // make bound inclusive
    ++prev;

    // check out bounds
    if (prev >= NumWords * kBitsPerWord) {
        return not_found;
    }

    // search first word
    size_t i = WhichWord(prev);
    WordT this_word = words_[i];

    // mask off bits below bound
    this_word &= (~static_cast<WordT>(0)) << WhichBit(prev);

    if (this_word != static_cast<WordT>(0)) {
        return i * kBitsPerWord + std::countr_zero(this_word);
    }

    // check subsequent words
    ++i;
    for (; i < NumWords; ++i) {
        this_word = words_[i];
        if (this_word != static_cast<WordT>(0)) {
            return i * kBitsPerWord + std::countr_zero(this_word);
        }
    }
    return not_found;
}

/// @brief Base class, specialization for a single word.
template <>
class BaseBitset<1> {
public:
    using WordT = unsigned long;

    constexpr BaseBitset() noexcept : word_(0) {}

    constexpr BaseBitset(unsigned long long val) noexcept : word_(val) {}

    static constexpr size_t WhichWord(size_t pos) noexcept {
        return pos / kBitsPerWord;
    }

    static constexpr size_t WhichByte(size_t pos) noexcept {
        return (pos % kBitsPerWord) / kCharBit;
    }

    static constexpr size_t WhichBit(size_t pos) noexcept {
        return pos % kBitsPerWord;
    }

    static constexpr WordT MaskBit(size_t pos) noexcept {
        return (static_cast<WordT>(1)) << WhichBit(pos);
    }

    constexpr WordT& GetWord([[maybe_unused]] size_t) noexcept {
        return word_;
    }

    constexpr WordT GetWord([[maybe_unused]] size_t) const noexcept {
        return word_;
    }

    constexpr WordT const* GetData() const noexcept {
        return &word_;
    }

    constexpr WordT& HiWord() noexcept {
        return word_;
    }

    constexpr WordT HiWord() const noexcept {
        return word_;
    }

    constexpr void DoAnd(BaseBitset<1> const& x) noexcept {
        word_ &= x.word_;
    }

    constexpr void DoOr(BaseBitset<1> const& x) noexcept {
        word_ |= x.word_;
    }

    constexpr void DoXor(BaseBitset<1> const& x) noexcept {
        word_ ^= x.word_;
    }

    constexpr void DoLeftShift(size_t shift) noexcept {
        word_ <<= shift;
    }

    constexpr void DoRightShift(size_t shift) noexcept {
        word_ >>= shift;
    }

    constexpr void DoFlip() noexcept {
        word_ = ~word_;
    }

    constexpr void DoSet() noexcept {
        word_ = ~static_cast<WordT>(0);
    }

    constexpr void DoReset() noexcept {
        word_ = 0;
    }

    constexpr bool IsEqual(BaseBitset<1> const& x) const noexcept {
        return word_ == x.word_;
    }

    template <size_t NumBits>
    constexpr bool AreAll() const noexcept {
        return word_ == (~static_cast<WordT>(0) >> (kBitsPerWord - NumBits));
    }

    constexpr bool IsAny() const noexcept {
        return word_ != 0;
    }

    constexpr size_t DoCount() const noexcept {
        return std::popcount(word_);
    }

    constexpr unsigned long DoToULong() const noexcept {
        return word_;
    }

    constexpr unsigned long long DoToULLong() const noexcept {
        return word_;
    }

    constexpr size_t DoFindFirst(size_t not_found) const noexcept {
        if (word_ != 0) {
            return std::countr_zero(word_);
        }
        return not_found;
    }

    constexpr size_t DoFindNext(size_t prev, size_t not_found) const noexcept {
        ++prev;
        if (prev >= kBitsPerWord) {
            return not_found;
        }

        WordT x = word_ >> prev;
        if (x != 0) {
            return std::countr_zero(x) + prev;
        }
        return not_found;
    }

    // >>> From libc++
    size_t HashCode() const noexcept {
        return word_;
    }

    // <<< From libc++

private:
    WordT word_;
};

/// @brief Base class, specialization for zero-length bitset
template <>
class BaseBitset<0> {
public:
    using WordT = unsigned long;

    constexpr BaseBitset() noexcept {}

    constexpr BaseBitset([[maybe_unused]] unsigned long long) noexcept {}

    static constexpr size_t WhichWord(size_t pos) noexcept {
        return pos / kBitsPerWord;
    }

    static constexpr size_t WhichByte(size_t pos) noexcept {
        return (pos % kBitsPerWord) / kCharBit;
    }

    static constexpr size_t WhichBit(size_t pos) noexcept {
        return pos % kBitsPerWord;
    }

    static constexpr WordT MaskBit(size_t pos) noexcept {
        return (static_cast<WordT>(1)) << WhichBit(pos);
    }

    // Here's the only place where users of zero-legth bitsets are penalized
    // This function is unreachable until _Unchecked methods are used
    [[noreturn]] WordT& GetWord([[maybe_unused]] size_t) noexcept {
        // Originally, here is `throw` statement, but it violates `noexcept` specification
        // throw std::out_of_range("BaseBitset::GetWord");
        LOG(INFO) << "Out of range in BaseBitset::GetWord (zero-lenght bitset).";
        __builtin_unreachable();
    }

    constexpr WordT GetWord([[maybe_unused]] size_t) const noexcept {
        return 0;
    }

    constexpr WordT HiWord() const noexcept {
        return 0;
    }

    constexpr void DoAnd([[maybe_unused]] BaseBitset<0> const&) noexcept {}

    constexpr void DoOr([[maybe_unused]] BaseBitset<0> const&) noexcept {}

    constexpr void DoXor([[maybe_unused]] BaseBitset<0> const&) noexcept {}

    constexpr void DoLeftShift([[maybe_unused]] size_t) noexcept {}

    constexpr void DoRightShift([[maybe_unused]] size_t) noexcept {}

    constexpr void DoFlip() noexcept {}

    constexpr void DoSet() noexcept {}

    constexpr void DoReset() noexcept {}

    constexpr bool IsEqual([[maybe_unused]] BaseBitset<0> const&) const noexcept {
        return true;
    }

    template <size_t NumBits>
    constexpr bool AreAll() const noexcept {
        return true;
    }

    constexpr bool IsAny() const noexcept {
        return false;
    }

    constexpr size_t DoCount() const noexcept {
        return 0;
    }

    constexpr unsigned long DoToULong() const noexcept {
        return 0;
    }

    constexpr unsigned long long DoToULLong() const noexcept {
        return 0;
    }

    constexpr size_t DoFindFirst([[maybe_unused]] size_t) const noexcept {
        return 0;
    }

    constexpr size_t DoFindNext([[maybe_unused]] size_t, [[maybe_unused]] size_t) const noexcept {
        return 0;
    }

    // >>> From libc++
    size_t HashCode() const noexcept {
        return 0;
    }

    // <<< From libc++
};

/// @brief Helper class to zero out the unused high-order bits in the highest word.
template <size_t ExtraBits>
class Sanitize {
public:
    using WordT = unsigned long;

    static constexpr void DoSanitize(WordT& val) noexcept {
        val &= ~((~static_cast<WordT>(0)) << ExtraBits);
    }
};

template <>
class Sanitize<0> {
public:
    using WordT = unsigned long;

    static constexpr void DoSanitize([[maybe_unused]] WordT) noexcept {}
};

template <size_t NumBits, bool = (NumBits < kBitsPerUll)>
class SanitizeVal {
public:
    static constexpr unsigned long long DoSanitizeVal(unsigned long long val) {
        return val;
    }
};

template <size_t NumBits>
class SanitizeVal<NumBits, true> {
public:
    static constexpr unsigned long long DoSanitizeVal(unsigned long long val) {
        return val & ~((~static_cast<unsigned long long>(0)) << NumBits);
    }
};

template <size_t NumBits>
class BitsetImpl : private BaseBitset<BitsetWords(NumBits)> {
public:
    using Base = BaseBitset<BitsetWords(NumBits)>;
    using WordT = unsigned long;

    // To allow STL-style method names
    // NOLINTBEGIN(readability-identifier-naming)
    class reference {
    public:
        friend class BitsetImpl;

        reference(reference const&) = default;

        ~reference() noexcept {}

        reference& operator=(bool x) noexcept {
            if (x) {
                *word_pt_ |= Base::MaskBit(bit_pos_);
            } else {
                *word_pt_ &= ~Base::MaskBit(bit_pos_);
            }
            return *this;
        }

        reference& operator=(reference const& j) noexcept {
            if (*(j.word_pt_) & Base::MaskBit(j.bit_pos_)) {
                *word_pt_ |= Base::MaskBit(bit_pos_);
            } else {
                *word_pt_ &= ~Base::MaskBit(bit_pos_);
            }
            return *this;
        }

        /// @brief Flips the bit
        bool operator~() const noexcept {
            return (*word_pt_ & Base::MaskBit(bit_pos_)) == 0;
        }

        operator bool() const noexcept {
            return (*word_pt_ & Base::MaskBit(bit_pos_)) != 0;
        }

        reference& flip() noexcept {
            *word_pt_ ^= Base::MaskBit(bit_pos_);
            return *this;
        }

    private:
        WordT* word_pt_;
        size_t bit_pos_;

        reference(BitsetImpl& b, size_t pos) noexcept {
            word_pt_ = &b.GetWord(pos);
            bit_pos_ = Base::WhichBit(pos);
        }
    };

    friend class reference;

    constexpr BitsetImpl() noexcept {}

    constexpr BitsetImpl(unsigned long long val) noexcept
        : Base(SanitizeVal<NumBits>::DoSanitizeVal(val)) {}

    template <typename CharT, typename Traits, typename Alloc>
    explicit BitsetImpl(std::basic_string<CharT, Traits, Alloc> const& s, size_t position = 0)
        : Base() {
        CheckInitialPosition(s, position);
        CopyFromString(s, position, std::basic_string<CharT, Traits, Alloc>::npos, CharT('0'),
                       CharT('1'));
    }

    template <typename CharT, typename Traits, typename Alloc>
    BitsetImpl(std::basic_string<CharT, Traits, Alloc> const& s, size_t position, size_t n)
        : Base() {
        CheckInitialPosition(s, position);
        CopyFromString(s, position, n, CharT('0'), CharT('1'));
    }

    template <typename CharT, typename Traits, typename Alloc>
    BitsetImpl(std::basic_string<CharT, Traits, Alloc> const& s, size_t position, size_t n,
               CharT zero, CharT one = CharT('1'))
        : Base() {
        CheckInitialPosition(s, position);
        CopyFromString(s, position, n, zero, one);
    }

    template <typename CharT>
    explicit BitsetImpl(
            CharT const* str,
            typename std::basic_string<CharT>::size_type n = std::basic_string<CharT>::npos,
            CharT zero = CharT('0'), CharT one = CharT('1'))
        : Base() {
        if (!str) {
            throw std::logic_error("BitsetImpl::BitsetImpl(CharT const*, ...)");
        }

        using Traits = typename std::basic_string<CharT>::traits_type;

        if (n == std::basic_string<CharT>::npos) {
            n = Traits::length(str);
        }
        CopyFromPtr<CharT, Traits>(str, n, 0, n, zero, one);
    }

    BitsetImpl<NumBits>& operator&=(BitsetImpl<NumBits> const& rhs) noexcept {
        this->DoAnd(rhs);
        return *this;
    }

    BitsetImpl<NumBits>& operator|=(BitsetImpl<NumBits> const& rhs) noexcept {
        this->DoOr(rhs);
        return *this;
    }

    BitsetImpl<NumBits>& operator^=(BitsetImpl<NumBits> const& rhs) noexcept {
        this->DoXor(rhs);
        return *this;
    }

    BitsetImpl<NumBits>& operator<<=(size_t position) noexcept {
        if (position < NumBits) [[likely]] {
            this->DoLeftShift(position);
            this->DoSanitize();
        } else {
            this->DoReset();
        }
        return *this;
    }

    BitsetImpl<NumBits>& operator>>=(size_t position) noexcept {
        if (position < NumBits) [[likely]] {
            this->DoRightShift(position);
        } else {
            this->DoReset();
        }
        return *this;
    }

    // These methods starting with underscore are SGI extensions
    BitsetImpl<NumBits>& _Unchecked_set(size_t pos) noexcept {
        this->GetWord(pos) |= Base::MaskBit(pos);
        return *this;
    }

    BitsetImpl<NumBits>& _Unchecked_set(size_t pos, int val) noexcept {
        if (val) {
            this->GetWord(pos) |= Base::MaskBit(pos);
        } else {
            this->GetWord(pos) &= ~Base::MaskBit(pos);
        }
        return *this;
    }

    BitsetImpl<NumBits>& _Unchecked_reset(size_t pos) noexcept {
        this->GetWord(pos) &= ~Base::MaskBit(pos);
        return *this;
    }

    BitsetImpl<NumBits>& _Unchecked_flip(size_t pos) noexcept {
        this->GetWord(pos) ^= Base::MaskBit(pos);
        return *this;
    }

    constexpr bool _Unchecked_test(size_t pos) const noexcept {
        return ((this->GetWord(pos) & Base::MaskBit(pos)) != static_cast<WordT>(0));
    }

    BitsetImpl<NumBits>& set() noexcept {
        this->DoSet();
        this->DoSanitize();
        return *this;
    }

    BitsetImpl<NumBits>& set(size_t position, bool val = true) {
        this->Check(position, "BitsetImpl::set");
        return _Unchecked_set(position, val);
    }

    BitsetImpl<NumBits>& reset() noexcept {
        this->DoReset();
        return *this;
    }

    BitsetImpl<NumBits>& reset(size_t position) {
        this->Check(position, "BitsetImpl::reset");
        return _Unchecked_reset(position);
    }

    BitsetImpl<NumBits>& flip() noexcept {
        this->DoFlip();
        this->DoSanitize();
        return *this;
    }

    BitsetImpl<NumBits>& flip(size_t position) {
        this->Check(position, "BitsetImpl::flip");
        return _Unchecked_flip(position);
    }

    BitsetImpl<NumBits> operator~() const noexcept {
        return BitsetImpl<NumBits>(*this).flip();
    }

    reference operator[](size_t position) {
        return reference(*this, position);
    }

    constexpr bool operator[](size_t position) const {
        return _Unchecked_test(position);
    }

    unsigned long to_ulong() const {
        return this->DoToULong();
    }

    unsigned long long to_ullong() const {
        return this->DoToULLong();
    }

    template <typename CharT, typename Traits, typename Alloc>
    std::basic_string<CharT, Traits, Alloc> to_string() const {
        std::basic_string<CharT, Traits, Alloc> result;
        CopyToString(result, CharT('0'), CharT('1'));
        return result;
    }

    template <typename CharT, typename Traits, typename Alloc>
    std::basic_string<CharT, Traits, Alloc> to_string(CharT zero, CharT one = CharT('1')) const {
        std::basic_string<CharT, Traits, Alloc> result;
        CopyToString(result, zero, one);
        return result;
    }

    template <typename CharT, typename Traits>
    std::basic_string<CharT, Traits, std::allocator<CharT>> to_string() const {
        return to_string<CharT, Traits, std::allocator<CharT>>();
    }

    template <typename CharT, typename Traits>
    std::basic_string<CharT, Traits, std::allocator<CharT>> to_string(
            CharT zero, CharT one = CharT('1')) const {
        return to_string<CharT, Traits, std::allocator<CharT>>(zero, one);
    }

    template <class CharT>
    std::basic_string<CharT, std::char_traits<CharT>, std::allocator<CharT>> to_string() const {
        return to_string<CharT, std::char_traits<CharT>, std::allocator<CharT>>();
    }

    template <class CharT>
    std::basic_string<CharT, std::char_traits<CharT>, std::allocator<CharT>> to_string(
            CharT zero, CharT one = CharT('1')) const {
        return to_string<CharT, std::char_traits<CharT>, std::allocator<CharT>>(zero, one);
    }

    std::string to_string() const {
        return to_string<char, std::char_traits<char>, std::allocator<char>>();
    }

    std::string to_string(char zero, char one = '1') const {
        return to_string<char, std::char_traits<char>, std::allocator<char>>(zero, one);
    }

    size_t count() const noexcept {
        return this->DoCount();
    }

    constexpr size_t size() const noexcept {
        return NumBits;
    }

    bool operator==(BitsetImpl<NumBits> const& rhs) const noexcept {
        return this->IsEqual(rhs);
    }

    bool test(size_t position) const {
        this->Check(position, "BitsetImpl::test");
        return _Unchecked_test(position);
    }

    bool all() const noexcept {
        return this->template AreAll<NumBits>();
    }

    bool any() const noexcept {
        return this->IsAny();
    }

    bool none() const noexcept {
        return !this->IsAny();
    }

    BitsetImpl<NumBits> operator<<(size_t position) const noexcept {
        return BitsetImpl<NumBits>(*this) <<= position;
    }

    BitsetImpl<NumBits> operator>>(size_t position) const noexcept {
        return BitsetImpl<NumBits>(*this) >>= position;
    }

    size_t _Find_first() const noexcept {
        return this->DoFindFirst(NumBits);
    }

    size_t _Find_next(size_t prev) const noexcept {
        return this->DoFindNext(prev, NumBits);
    }

    // NOLINTEND(readability-identifier-naming)

private:
    friend struct std::hash<BitsetImpl>;

    template <typename CharT, typename Traits, typename Alloc>
    void CheckInitialPosition(std::basic_string<CharT, Traits, Alloc> const& s, size_t position) {
        if (position > s.size()) {
            std::ostringstream ss;
            ss << "BitsetImpl::BitsetImpl: position (which is " << position
               << ") > s.size() (which is " << s.size() << ")";
            throw std::out_of_range(ss.str());
        }
    }

    void Check(size_t position, char const* s) const {
        if (position >= NumBits) {
            std::ostringstream ss;
            ss << s << ": position (which is " << position << ") >= NumBits (which is " << NumBits
               << ")";
            throw std::out_of_range(ss.str());
        }
    }

    void DoSanitize() noexcept {
        using SanitizeT = Sanitize<NumBits % kBitsPerWord>;
        SanitizeT::DoSanitize(this->HiWord());
    }

    // >>> From libc++
    size_t HashCode() const noexcept {
        return Base::HashCode();
    }

    // <<< From libc++

    template <typename CharT, typename Traits>
    void CopyFromPtr(CharT const*, size_t, size_t, size_t, CharT, CharT);

    template <typename CharT, typename Traits, typename Alloc>
    void CopyFromString(std::basic_string<CharT, Traits, Alloc> const& s, size_t pos, size_t n,
                        CharT zero, CharT one) {
        CopyFromPtr<CharT, Traits>(s.data(), s.size(), pos, n, zero, one);
    }

    template <typename CharT, typename Traits, typename Alloc>
    void CopyToString(std::basic_string<CharT, Traits, Alloc>&, CharT, CharT) const;

    template <typename CharT, typename Traits, size_t NumBits2>
    friend std::basic_istream<CharT, Traits>& operator>>(std::basic_istream<CharT, Traits>&,
                                                         BitsetImpl<NumBits2>&);

    template <typename CharT, typename Traits, size_t NumBits2>
    friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>&,
                                                         BitsetImpl<NumBits2> const&);
};

// Definitions of non-inline member functions.
template <size_t NumBits>
template <typename CharT, typename Traits>
void BitsetImpl<NumBits>::CopyFromPtr(CharT const* s, size_t len, size_t pos, size_t n, CharT zero,
                                      CharT one) {
    reset();
    size_t const nbits = std::min({NumBits, n, static_cast<size_t>(len - pos)});
    for (size_t i{nbits}; i > 0; --i) {
        CharT const c = s[pos + nbits - i];
        if (Traits::eq(c, zero)) {
            // Here's already zero
        } else if (Traits::eq(c, one)) {
            _Unchecked_set(i - 1);
        } else {
            throw std::invalid_argument("BitsetImpl::CopyFromPtr");
        }
    }
}

template <size_t NumBits>
template <typename CharT, typename Traits, typename Alloc>
void BitsetImpl<NumBits>::CopyToString(std::basic_string<CharT, Traits, Alloc>& s, CharT zero,
                                       CharT one) const {
    s.assign(NumBits, zero);
    size_t n = this->_Find_first();
    while (n < NumBits) {
        s[NumBits - n - 1] = one;
        n = _Find_next(n);
    }
}

template <size_t NumBits>
inline BitsetImpl<NumBits> operator&(BitsetImpl<NumBits> const& x,
                                     BitsetImpl<NumBits> const& y) noexcept {
    BitsetImpl<NumBits> result(x);
    result &= y;
    return result;
}

template <size_t NumBits>
inline BitsetImpl<NumBits> operator|(BitsetImpl<NumBits> const& x,
                                     BitsetImpl<NumBits> const y) noexcept {
    BitsetImpl<NumBits> result(x);
    result |= y;
    return result;
}

template <size_t NumBits>
inline BitsetImpl<NumBits> operator^(BitsetImpl<NumBits> const& x,
                                     BitsetImpl<NumBits> const& y) noexcept {
    BitsetImpl<NumBits> result(x);
    result ^= y;
    return result;
}

template <typename CharT, typename Traits, size_t NumBits>
std::basic_istream<CharT, Traits>& operator>>(std::basic_istream<CharT, Traits>& is,
                                              BitsetImpl<NumBits>& x) {
    using CharType = Traits::char_type;
    using IStreamType = std::basic_istream<CharT, Traits>;
    using IOSBase = IStreamType::ios_base;

    struct Buffer {
        static constexpr bool UseAlloca() {
            return NumBits <= 256;
        }

        explicit Buffer(CharT* p) : ptr(p) {}

        ~Buffer() {
            if constexpr (!UseAlloca()) {
                delete[] ptr;
            }
        }

        CharT* const ptr;
    };

    CharT* pt;

    if constexpr (Buffer::UseAlloca()) {
        pt = static_cast<CharT*>(alloca(NumBits));
    } else {
        pt = new CharT[NumBits];
    }
    Buffer const buf(pt);

    CharType const zero = is.widen('0');
    CharType const one = is.widen('1');

    typename IOSBase::iostate state = IOSBase::goodbit;
    typename IStreamType::sentry sentry(is);
    if (sentry) {
        try {
            for (size_t i{NumBits}; i > 0; --i) {
                static typename Traits::int_type eof = Traits::eof();

                typename Traits::int_type c1 = is.rdbuf()->sbumpc();
                if (Traits::eq_int_type(c1, eof)) {
                    state |= IOSBase::eofbit;
                    break;
                } else {
                    CharType const c2 = Traits::to_char_type(c1);
                    if (Traits::eq(c2, zero)) {
                        *pt++ = zero;
                    } else if (Traits::eq(c2, one)) {
                        *pt++ = one;
                    } else if (Traits::eq_int_type(is.rdbuf()->sputbackc(c2), eof)) {
                        state |= IOSBase::failbit;
                        break;
                    }
                }
            }
        } catch (...) {
            is.setstate(IOSBase::badbit);
        }
    }

    if constexpr (NumBits) {
        if (size_t len = pt - buf.ptr) {
            x.template CopyFromPtr<CharT, Traits>(buf.ptr, len, 0, len, zero, one);
        } else {
            state |= IOSBase::failbit;
        }
    }
    if (state) {
        is.setstate(state);
    }
    return is;
}

template <typename CharT, typename Traits, size_t NumBits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
                                              BitsetImpl<NumBits> const& x) {
    std::basic_string<CharT, Traits> tmp;

    std::ctype<CharT> const& ct = std::use_facet<std::ctype<CharT>>(os.getloc());
    x.CopyToString(tmp, ct.widen('0'), ct.widen('1'));
    return os << tmp;
}

template <typename Bitset>
concept HasExtensions = requires(Bitset bs) {
    bs._Find_first();
    bs._Find_next(0);
};

template <size_t S>
struct BitsetSelector {
    using Type = BitsetImpl<S>;
};

template <size_t S>
    requires HasExtensions<std::bitset<S>>
struct BitsetSelector<S> {
    using Type = std::bitset<S>;
};

}  // namespace bitset_impl

template <size_t S>
using Bitset = typename bitset_impl::BitsetSelector<S>::Type;

}  // namespace model

/// @note This hash is equal to hash of @b libc++ std::bitset with the same data.
/// It's @b not equal to hash of @b libstdc++ std::bitset with the same data.
template <size_t NumBits>
struct std::hash<model::bitset_impl::BitsetImpl<NumBits>> {
    size_t operator()(model::bitset_impl::BitsetImpl<NumBits> const& b) const noexcept {
        return b.HashCode();
    }
};
