/*
 * Based on atomic_bitvector by Erik Garrison (Apache 2.0)
 * https://github.com/ekg/atomicbitvector
 *
 * Simplified version without iterator support.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>

namespace util {

class AtomicBitVector {
public:
    AtomicBitVector(size_t size)
        : size_(size),
          num_blocks_((size + kBitsPerBlock - 1) / kBitsPerBlock),
          data_(std::make_unique<std::atomic<BlockType>[]>(num_blocks_)) {}

    // Set bit to true. Returns previous value.
    bool Set(size_t idx, std::memory_order order = std::memory_order_seq_cst) {
        assert(idx < size_);
        BlockType mask = kOne << BitOffset(idx);
        return data_[BlockIndex(idx)].fetch_or(mask, order) & mask;
    }

    // Set bit to false. Returns previous value.
    bool Reset(size_t idx, std::memory_order order = std::memory_order_seq_cst) {
        assert(idx < size_);
        BlockType mask = kOne << BitOffset(idx);
        return data_[BlockIndex(idx)].fetch_and(~mask, order) & mask;
    }

    // Set bit to given value. Returns previous value.
    bool Set(size_t idx, bool value, std::memory_order order = std::memory_order_seq_cst) {
        return value ? Set(idx, order) : Reset(idx, order);
    }

    // Read bit value.
    bool Test(size_t idx, std::memory_order order = std::memory_order_seq_cst) const {
        assert(idx < size_);
        BlockType mask = kOne << BitOffset(idx);
        return data_[BlockIndex(idx)].load(order) & mask;
    }

    bool operator[](size_t idx) const {
        return Test(idx);
    }

    constexpr size_t Size() const noexcept {
        return size_;
    }

private:
#if (ATOMIC_LLONG_LOCK_FREE == 2)
    using BlockType = unsigned long long;
#elif (ATOMIC_LONG_LOCK_FREE == 2)
    using BlockType = unsigned long;
#else
    using BlockType = unsigned int;
#endif

    static constexpr size_t kBitsPerBlock = std::numeric_limits<BlockType>::digits;
    static constexpr BlockType kOne = 1;

    static constexpr size_t BlockIndex(size_t bit) {
        return bit / kBitsPerBlock;
    }

    static constexpr size_t BitOffset(size_t bit) {
        return bit % kBitsPerBlock;
    }

    size_t size_;
    size_t num_blocks_;
    std::unique_ptr<std::atomic<BlockType>[]> data_;
};

}  // namespace util
