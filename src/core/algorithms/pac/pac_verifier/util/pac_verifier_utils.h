#pragma once

namespace algos::pac_verifier {
struct EpsilonDelta {
    double epsilon;
    double delta;
};

// Threshold for floating-point comparison of distances
constexpr static double kDistThreshold = 1e-12;
}  // namespace algos::pac_verifier
