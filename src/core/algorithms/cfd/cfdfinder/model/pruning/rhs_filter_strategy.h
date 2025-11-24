#pragma once
#include <algorithm>
#include <cstddef>

#include "algorithms/cfd/cfdfinder/model/pruning/support_independent_strategy.h"
#include "config/indices/type.h"

namespace algos::cfdfinder {

class RhsFilterPruning : public SupportIndependentPruning {
private:
    config::IndicesType possible_rhs_;

public:
    RhsFilterPruning(size_t pattern_threshold, double min_support_gain,
                     double max_level_support_drop, double min_confidence,
                     config::IndicesType possible_rhs)
        : SupportIndependentPruning(pattern_threshold, min_support_gain, max_level_support_drop,
                                    min_confidence),
          possible_rhs_(std::move(possible_rhs)) {}

    bool ContinueGeneration(PatternTableau const& current_tableau) override {
        if (!std::binary_search(possible_rhs_.begin(), possible_rhs_.end(),
                                current_candidate_.rhs_)) {
            return false;
        }
        return SupportIndependentPruning::ContinueGeneration(current_tableau);
    }
};

}  // namespace algos::cfdfinder
