#pragma once

#include <enum.h>

namespace algos::dc {
BETTER_ENUM(MeasureType, char, G1 = 0, G1_NORM, G2);
}

template <>
struct std::hash<algos::dc::MeasureType> {
    size_t operator()(algos::dc::MeasureType measure) const noexcept {
        return static_cast<std::size_t>(measure._value);
    }
};