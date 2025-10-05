#pragma once

#include <set>
#include <string>
#include <vector>

#include "config/indices/type.h"
#include "support_independent_strategy.h"

namespace algos::cfdfinder {

class RhsFilterPruning : public SupportIndependentPruning {
private:
    config::IndicesType possible_rhs_;

public:
    RhsFilterPruning(size_t pattern_threshold, double min_support_gain,
                     double max_level_support_drop, double min_confidence, size_t num_attributes,
                     config::IndicesType possible_rhs)
        : SupportIndependentPruning(pattern_threshold, min_support_gain, max_level_support_drop,
                                    min_confidence, num_attributes),
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