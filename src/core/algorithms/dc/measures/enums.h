#pragma once

namespace algos::dc {
enum class MeasureType { kG1 = 0, kG1Norm, kG2 };
}

template <>
struct std::hash<algos::dc::MeasureType> {
    size_t operator()(algos::dc::MeasureType val) const noexcept {
        return static_cast<std::size_t>(val);
    }
};