#pragma once

#include <memory>
#include <vector>

#include "algorithms/mde/decision_boundaries/decision_boundary.h"
#include "util/get_preallocated_vector.h"

namespace algos::hymde::record_match_indexes::calculators {
template <typename DecisionBoundaryType, typename ResultType>
std::vector<std::shared_ptr<model::mde::decision_boundaries::DecisionBoundary>>
ToDecisionBoundaries(std::vector<ResultType> comparison_results) {
    std::vector<std::shared_ptr<model::mde::decision_boundaries::DecisionBoundary>>
            decision_boundaries = util::GetPreallocatedVector<
                    std::shared_ptr<model::mde::decision_boundaries::DecisionBoundary>>(
                    comparison_results.size());
    for (ResultType const& result : comparison_results) {
        decision_boundaries.push_back(std::make_shared<DecisionBoundaryType>(result));
    }
    return decision_boundaries;
}
}  // namespace algos::hymde::record_match_indexes::calculators
