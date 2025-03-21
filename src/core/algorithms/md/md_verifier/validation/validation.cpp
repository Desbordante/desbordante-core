#include "algorithms/md/md_verifier/validation/validation.h"

#include "algorithms/md/md_verifier/similarities/similarity_calculator.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::md {
bool MDValidationCalculator::ProbeColumnSimilarityClassifier(
        model::md::ColumnSimilarityClassifier const& classifier, model::Index left_row_index,
        model::Index right_row_index, bool is_rhs) {
    algos::md::MDVerifierColumnMatch const& column_match =
            column_matches_[classifier.GetColumnMatchIndex()];
    model::md::DecisionBoundary decision_boundary = classifier.GetDecisionBoundary();
    model::Index left_col_index = column_match.left_col_index;
    model::Index right_col_index = column_match.right_col_index;
    std::shared_ptr<SimilarityMeasure> measure = column_match.measure;
    if (left_typed_data_->GetColumnData(left_col_index).GetTypeId() !=
        right_typed_data_->GetColumnData(right_col_index).GetTypeId()) {
        throw std::runtime_error("Unable to calculate similarity of columns with different types");
    }

    model::md::Similarity similarity = SimilarityCalculator::Calculate(
            left_typed_data_->GetColumnData(left_col_index).GetValue(left_row_index),
            right_typed_data_->GetColumnData(right_col_index).GetValue(right_row_index),
            left_typed_data_->GetColumnData(left_col_index).GetTypeId(), measure);

    if (is_rhs && similarity < decision_boundary) {
        true_rhs_decision_boundary = std::min(similarity, true_rhs_decision_boundary);
        highlights_.RegisterHighlight(left_row_index, right_row_index, column_match, similarity,
                                      decision_boundary);
    }

    return similarity >= decision_boundary;
}

MDValidationCalculator::MDValidationCalculator(
        config::InputTable left_table, config::InputTable right_table,
        std::vector<MDVerifierColumnMatch> column_matches,
        std::vector<model::md::ColumnSimilarityClassifier> lhs_column_similarity_classifiers,
        model::md::ColumnSimilarityClassifier rhs_column_similarity_classifier)
    : left_typed_data_(model::ColumnLayoutTypedRelationData::CreateFrom(*left_table, true)),
      column_matches_(column_matches),
      lhs_column_similarity_classifiers_(lhs_column_similarity_classifiers),
      rhs_column_similarity_classifier_(rhs_column_similarity_classifier),
      true_rhs_decision_boundary(rhs_column_similarity_classifier.GetDecisionBoundary()),
      holds_(true) {
    if (right_table != nullptr) {
        right_typed_data_ = model::ColumnLayoutTypedRelationData::CreateFrom(*right_table, true);
    } else {
        right_typed_data_ = left_typed_data_;
    }
}

void MDValidationCalculator::Validate() {
    if (!holds_) {
        return;
    }

    size_t left_num_rows = left_typed_data_->GetNumRows();
    size_t right_num_rows = right_typed_data_->GetNumRows();

    for (model::Index left_row_index = 0; left_row_index < left_num_rows; ++left_row_index) {
        for (model::Index right_row_index = 0; right_row_index < right_num_rows;
             ++right_row_index) {
            bool holds_for_lhs = std::all_of(
                    lhs_column_similarity_classifiers_.begin(),
                    lhs_column_similarity_classifiers_.end(),
                    [this, left_row_index,
                     right_row_index](model::md::ColumnSimilarityClassifier const& lhs_classifier) {
                        return ProbeColumnSimilarityClassifier(lhs_classifier, left_row_index,
                                                               right_row_index);
                    });
            if (!holds_for_lhs) {
                continue;
            }
            bool holds_for_rhs = ProbeColumnSimilarityClassifier(
                    rhs_column_similarity_classifier_, left_row_index, right_row_index, true);
            holds_ = holds_ && holds_for_rhs;
        }
    }
}
}  // namespace algos::md
