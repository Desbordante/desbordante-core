#pragma once

#include <vector>

#include "algorithms/md/column_match.h"
#include "algorithms/md/column_similarity_classifier.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/md_verifier/highlights/highlights.h"
#include "algorithms/md/md_verifier/md_verifier_column_match.h"
#include "algorithms/md/md_verifier/similarities/similarities.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::md {

class MDValidationCalculator {
private:
    std::shared_ptr<model::ColumnLayoutTypedRelationData> left_typed_data_;
    std::shared_ptr<model::ColumnLayoutTypedRelationData> right_typed_data_;
    std::vector<MDVerifierColumnMatch> column_matches_;
    std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers_;
    model::md::ColumnSimilarityClassifier rhs_column_similarity_classifier_;

    model::md::DecisionBoundary true_rhs_decision_boundary_;
    MDHighlights highlights_;

    bool holds_;

    bool ProbeColumnSimilarityClassifier(model::md::ColumnSimilarityClassifier const& classifier,
                                         model::Index left_row, model::Index right_row,
                                         bool is_rhs = false);

public:
    MDValidationCalculator(
            config::InputTable left_table, config::InputTable right_table,
            std::vector<MDVerifierColumnMatch> column_matches,
            std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers,
            model::md::ColumnSimilarityClassifier rhs_column_similarity_classifier);
    void Validate();

    bool Holds() const {
        return holds_;
    }

    model::md::DecisionBoundary GetTrueDecisionBoundary() const {
        return true_rhs_decision_boundary_;
    }

    MDHighlights GetHighlights() const {
        return highlights_;
    }
};
}  // namespace algos::md
