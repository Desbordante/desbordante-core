#pragma once

#include <memory>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "algorithms/dc/verifier/dc_verifier.h"

namespace algos::dc {

enum class MeasureType { G1, G1_NORM, G2 };

class Measure {
private:
    size_t rel_size_;
    std::shared_ptr<DCVerifier> verifier_;
    std::unordered_map<MeasureType, double (Measure::*)()> type_to_method_map_;

    double G1();

    double G1_NORM();

    double G2();

    std::vector<size_t> GetFrequencies() const;

public:
    Measure(std::shared_ptr<algos::DCVerifier> verifier);

    // Get a certain measure by its type
    double Get(MeasureType type);
};

}  // namespace algos::dc
