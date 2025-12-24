#pragma once

#include <magic_enum/magic_enum.hpp>

namespace algos {
enum class PfdErrorMeasure : char { kPerTuple = 0, kPerValue };

enum class AfdErrorMeasure : char { kG1 = 0, kPdep, kTau, kMuPlus, kRho };
}  // namespace algos
