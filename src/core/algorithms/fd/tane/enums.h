#pragma once

#include "core/util/export.h"

namespace algos {
enum class DESBORDANTE_EXPORT PfdErrorMeasure : char { kPerTuple = 0, kPerValue };

enum class DESBORDANTE_EXPORT AfdErrorMeasure : char { kG1 = 0, kPdep, kTau, kMuPlus, kRho };
}  // namespace algos
