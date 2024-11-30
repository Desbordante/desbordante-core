#include "bitset_extensions.h"

#include <bit>
#include <bitset>

namespace util::bitset_extensions {

CONSTEXPR_IF_VECTOR_IS_CONSTEXPR unsigned char GetByte(unsigned long long val, size_t byte_num) {
    return (val & kBytes[byte_num]) >> (byte_num * 8ul);
}

size_t FindFirstFixedWidth(std::bitset<kWidth> const& bs) {
    if (bs.none()) {
        return kWidth;
    }
    unsigned long long val = bs.to_ullong();
    for (size_t byte_idx{0ul}; byte_idx < kNumBytes; ++byte_idx) {
        auto byte = GetByte(val, byte_idx);
        if (byte > 0ul) {
            return byte_idx * 8ul + std::countr_zero(byte);
        }
    }
    __builtin_unreachable();
}

size_t FindNextFixedWidth(std::bitset<kWidth> const& bs, size_t pos) {
    if (bs.none()) {
        return kWidth;
    }
    unsigned long long val = bs.to_ullong();
    size_t start_byte = pos / 8ul;
    size_t bit_pos = pos % 8ul;
    for (size_t byte_idx{start_byte}; byte_idx < kNumBytes; ++byte_idx) {
        auto byte = GetByte(val, byte_idx);
        if (byte_idx == start_byte) {
            byte &= kFirstBits[bit_pos];
        }
        if (byte > 0ul) {
            return byte_idx * 8ul + std::countr_zero(byte);
        }
    }
    return kWidth;
}

}  // namespace util::bitset_extensions
