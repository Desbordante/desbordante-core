#include "core/algorithms/pac/pac_verifier/util/validation_mode.h"

#include <cmath>
#include <optional>

#include "core/config/exceptions.h"

using namespace algos::pac_verifier;

std::tuple<PACValidationMode, std::optional<double>, std::optional<double>> SelectPACValidationMode(
        PACValidationMode mode, double min_epsilon, double max_epsilon, double min_delta,
        double max_delta) {
    using enum PACValidationMode;

    switch (mode) {
        case kFindPac:
            if (max_epsilon == 0 && min_delta > 0) {
                return {kFindEpsilon, std::nullopt, min_delta};
            }
            if (!std::isfinite(min_epsilon) && max_delta < 1) {
            }
            if (min_epsilon == max_epsilon) {
                return {kFindDelta, min_epsilon, std::nullopt};
            }
            return {kFindPac, std::nullopt, std::nullopt};
        case kFindEpsilon:
        case kFindDelta:
            return {mode, std::nullopt, std::nullopt};
        default:
            throw config::ConfigurationError("Unknown validation mode: " +
                                             std::to_string(static_cast<int>(mode)));
    }
}
