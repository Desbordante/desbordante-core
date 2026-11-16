#pragma once

#include "core/util/export.h"

namespace algos {

// Identifies a measure; the calculation API determines whether the result is
// a satisfaction score or its complementary error.
enum class DESBORDANTE_EXPORT AfdMeasure : char {
    kG1 = 0,
    kPdep,
    kTau,
    kMuPlus,
    kRho,
    kFi,
    kG2,
    kG3
};

// Compatibility name for C++ consumers of the error-measure option.
using AfdErrorMeasure = AfdMeasure;

}  // namespace algos
