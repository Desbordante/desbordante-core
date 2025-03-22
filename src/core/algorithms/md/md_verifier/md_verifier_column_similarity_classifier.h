#pragma once

#include "algorithms/md/md_verifier/md_verifier_column_match.h"

namespace algos::md {
class MDVerifierColumnSimilarityClassifier {
private:
    algos::md::MDVerifierColumnMatch column_match_;
    model::md::DecisionBoundary decision_boundary_;

public:
    MDVerifierColumnSimilarityClassifier() = default;

    MDVerifierColumnSimilarityClassifier(algos::md::MDVerifierColumnMatch column_match,
                                         model::md::DecisionBoundary decision_boundary)
        : column_match_(column_match), decision_boundary_(decision_boundary) {}

    MDVerifierColumnSimilarityClassifier(model::Index left_col_index, model::Index right_col_index,
                                         std::shared_ptr<algos::md::SimilarityMeasure> measure,
                                         model::md::DecisionBoundary decision_boundary)
        : column_match_(left_col_index, right_col_index, measure),
          decision_boundary_(decision_boundary) {}

    algos::md::MDVerifierColumnMatch const& GetColumnMatch() const {
        return column_match_;
    }

    model::md::DecisionBoundary GetDecisionBoundary() const {
        return decision_boundary_;
    }
};
}  // namespace algos::md