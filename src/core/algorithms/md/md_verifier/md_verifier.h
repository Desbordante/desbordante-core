#pragma once

#include "algorithms/algorithm.h"
#include "algorithms/md/column_match.h"
#include "algorithms/md/decision_boundary.h"
#include "algorithms/md/md_algorithm.h"
#include "algorithms/md/md_verifier/highlights/highlights.h"
#include "algorithms/md/md_verifier/md_verifier_column_similarity_classifier.h"
#include "algorithms/md/md_verifier/validation/validation.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"
#include "model/table/relational_schema.h"

namespace algos::md {

class MDVerifier : public MdAlgorithm {
private:
    config::InputTable left_table_;
    config::InputTable right_table_;

    std::shared_ptr<RelationalSchema> left_schema_;
    std::shared_ptr<RelationalSchema> right_schema_;

    std::shared_ptr<model::MD> input_md_;

    std::vector<MDVerifierColumnSimilarityClassifier> lhs_;
    MDVerifierColumnSimilarityClassifier rhs_;

    bool md_holds_ = false;
    model::md::DecisionBoundary true_rhs_decision_boundary_;
    MDHighlights highlights_;

    void ResetStateMd() override;

    void RegisterOptions();
    void VerifyMD();

    model::MD BuildMD(std::vector<MDVerifierColumnSimilarityClassifier> const& lhs,
                      MDVerifierColumnSimilarityClassifier const& rhs);

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
        return true_rhs_decision_boundary_;
    }

    std::vector<MDHighlights::Highlight> GetHighlights() const {
        return highlights_.GetHighlights();
    }

    model::MD const& GetProvidedMD() const {
        return *input_md_;
    }

    std::list<model::MD> const& GetMDSuggestion() const noexcept {
        return MdList();
    }
};

}  // namespace algos::md
