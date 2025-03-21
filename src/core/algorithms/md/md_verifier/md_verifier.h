#pragma once

#include "algorithms/algorithm.h"
#include "algorithms/md/column_match.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/md_verifier/highlights/highlights.h"
#include "algorithms/md/md_verifier/md_verifier_column_match.h"
#include "algorithms/md/md_verifier/validation/validation.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"

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

class MDVerifier : public Algorithm {
private:
    config::InputTable left_table_;
    config::InputTable right_table_;

    std::vector<MDVerifierColumnSimilarityClassifier> lhs_;
    MDVerifierColumnSimilarityClassifier rhs_;

    bool md_holds_ = false;
    model::md::DecisionBoundary true_rhs_decision_boundary;
    MDHighlights highlights_;

    void ResetState() final;
    void RegisterOptions();
    void VerifyMD();

protected:
    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    MDVerifier();

    bool GetResult() const {
        return md_holds_;
    }

    model::md::DecisionBoundary GetRHSSuggestion() const {
        return true_rhs_decision_boundary;
    }

    std::vector<MDHighlights::Highlight> GetHighlights() const {
        return highlights_.GetHighlights();
    }
};

}  // namespace algos::md
