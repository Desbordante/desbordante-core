#pragma once

#include <optional>
#include <tuple>

#include "core/util/export.h"

namespace algos::pac_verifier {
enum class DESBORDANTE_EXPORT PACValidationMode { kFindPac, kFindEpsilon, kFindDelta };

/// @returns Selected mode, epsilon override, delta override
std::tuple<PACValidationMode, std::optional<double>, std::optional<double>> SelectPACValidationMode(
        PACValidationMode mode, double min_epsilon, double max_epsilon, double min_delta,
        double max_delta);
}  // namespace algos::pac_verifier
