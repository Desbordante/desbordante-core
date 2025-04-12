#pragma once

#include <functional>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "core/algorithms/dc/measures/enums.h"

namespace algos {
class DCVerifier;
}

namespace algos::dc {

class Measure {
private:
    size_t rel_size_;
    std::shared_ptr<algos::DCVerifier> verifier_;
    std::unordered_map<MeasureType, double (Measure::*)(void)> type_to_method_map_;

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
