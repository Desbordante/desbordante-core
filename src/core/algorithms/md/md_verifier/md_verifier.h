#pragma once

#include <optional>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/md/column_match.h"
#include "core/algorithms/md/decision_boundary.h"
#include "core/algorithms/md/md_algorithm.h"
#include "core/algorithms/md/md_verifier/cmptr.h"
#include "core/algorithms/md/md_verifier/column_similarity_classifier.h"
#include "core/algorithms/md/md_verifier/highlights/highlights.h"
#include "core/algorithms/md/md_verifier/validation/validation.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/config/thread_number/type.h"
#include "core/model/table/relational_schema.h"

namespace algos::md {

class MDVerifier : public Algorithm {
private:
    config::InputTable left_table_;
    config::InputTable right_table_;

    std::shared_ptr<RelationalSchema> left_schema_;
    std::shared_ptr<RelationalSchema> right_schema_;

    std::optional<model::MD> input_md_;
    std::optional<model::MD> md_suggestion_;

    std::vector<ColumnSimilarityClassifier> lhs_;
    ColumnSimilarityClassifier rhs_;

    bool md_holds_ = false;
    model::md::DecisionBoundary true_rhs_decision_boundary_;

    std::shared_ptr<MDHighlights> highlights_;

    config::ThreadNumType threads_;

    void ResetState() override;

    void RegisterOptions();
    void VerifyMD();

    MDValidationCalculator CreateValidator() const;

    model::MD BuildMD(std::vector<ColumnSimilarityClassifier> const& lhs,
                      ColumnSimilarityClassifier const& rhs);

protected:
    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    MDVerifier();

    bool GetResult() const {
        return md_holds_;
    }

    model::md::DecisionBoundary GetTrueRhsDecisionBoundary() const {
        return true_rhs_decision_boundary_;
    }

    model::MD const& GetMDSuggestion() const {
        if (md_suggestion_.has_value()) {
            return *md_suggestion_;
        }

        throw std::runtime_error("MD suggestion is not initialized: execute algorithm first");
    }

    model::MD GetInputMD() const {
        if (input_md_.has_value()) {
            return *input_md_;
        }

        throw std::runtime_error("Input MD is not initialized: execute algorithm first");
    }

    std::vector<MDHighlights::Highlight> const& GetHighlights() const {
        return highlights_->GetHighlights();
    }

    std::vector<MDHighlights::Highlight> GetHighlightsCopy() const {
        return highlights_->GetHighlights();
    }
};

}  // namespace algos::md
