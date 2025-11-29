#pragma once

#include "core/algorithms/md/hymd/preprocessing/column_matches/column_match.h"
#include "core/algorithms/md/md_verifier/cmptr.h"

namespace algos::md {
class ColumnSimilarityClassifier {
private:
    CMPtr column_match_;
    model::md::DecisionBoundary decision_boundary_;

public:
    ColumnSimilarityClassifier() = default;

    ColumnSimilarityClassifier(CMPtr column_match, model::md::DecisionBoundary decision_boundary)
        : column_match_(std::move(column_match)), decision_boundary_(decision_boundary) {}

    CMPtr const& GetColumnMatch() const {
        return column_match_;
    }

    model::md::DecisionBoundary GetDecisionBoundary() const {
        return decision_boundary_;
    }
};
}  // namespace algos::md
