#pragma once

#include <memory>
#include <numeric>
#include <unordered_map>
#include <vector>

#include <enum.h>

#include "algorithms/dc/DCVerifier/dc_verifier.h"

namespace algos::dc {
BETTER_ENUM(MeasureType, char, G1 = 0, G1_NORM, G2);
}

template <>
struct std::hash<algos::dc::MeasureType> {
    size_t operator()(algos::dc::MeasureType measure) const noexcept {
        return static_cast<std::size_t>(measure._value);
    }
};

namespace algos::dc {

class Measure {
private:
    size_t rel_size_;
    std::shared_ptr<::algos::DCVerifier> verifier_;
    std::unordered_map<MeasureType, double (Measure::*)()> type_to_method_map_;

    double G1();

    double G1_NORM();

    double G2();

    std::vector<size_t> GetFrequencies() const;

public:
    Measure(std::shared_ptr<algos::DCVerifier> verifier);
    Measure(Measure const&) = default;
    Measure(Measure&&) = default;
    Measure& operator=(Measure const&) = default;
    Measure& operator=(Measure&&) = default;

    // Get a certain measure by its type
    double Get(MeasureType type);
};

}  // namespace algos::dc
