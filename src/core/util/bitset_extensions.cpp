#include "bitset_extensions.h"

#include <bit>
#include <bitset>

namespace util::bitset_extensions {

CONSTEXPR_IF_VECTOR_IS_CONSTEXPR unsigned char GetByte(unsigned long long val, size_t byte_num) {
    return (val & kBytes[byte_num]) >> (byte_num * 8);
}

size_t FindFirstFixedWidth(std::bitset<kWidth> const& bs) {
    if (bs.none()) {
        return kWidth;
    }
    unsigned long long val = bs.to_ullong();
    for (size_t byte_idx{0}; byte_idx < kNumBytes; ++byte_idx) {
        auto byte = GetByte(val, byte_idx);
        if (byte > 0) {
            return byte_idx * 8 + std::countr_zero(byte);
        }
    }
    __builtin_unreachable();
}

size_t FindNextFixedWidth(std::bitset<kWidth> const& bs, size_t pos) {
    if (bs.none()) {
        return kWidth;
    }
    unsigned long long val = bs.to_ullong();
    size_t start_byte = pos / 8;
    size_t bit_pos = pos % 8;
    for (size_t byte_idx{start_byte}; byte_idx < kNumBytes; ++byte_idx) {
        auto byte = GetByte(val, byte_idx);
        if (byte > 0) {
            if (byte_idx > start_byte) {
                return byte_idx * 8 + std::countr_zero(byte);
            } else {
                size_t leading_zeros = std::countl_zero(byte);
                if (leading_zeros < 7 - bit_pos) {
                    std::bitset<8> bs{byte};
                    for (size_t i{bit_pos + 1}; i < 8; ++i) {
                        if (bs[i]) {
                            return start_byte * 8 + i;
                        }
                    }
                }
            }
        }
    }
    return kWidth;
}

}  // namespace util::bitset_extensions
