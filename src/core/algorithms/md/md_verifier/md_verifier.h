#pragma once

#include "algorithms/algorithm.h"
#include "algorithms/md/md_verifier/similarities/similarities.h"
#include "algorithms/md/md_verifier/thresholds_type.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::md {

class MDVerifier : public Algorithm {
private:
    config::InputTable input_table_;

    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;
    ThresholdsType lhs_thresholds_;
    ThresholdsType rhs_thresholds_;
    SimilaritiesType lhs_similarity_measures_;
    SimilaritiesType rhs_similarity_measures_;

    config::EqNullsType is_null_equal_null_;
    bool dist_from_null_is_infinity_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> relation_;

    bool md_holds_ = false;

    void InitDefaultThresholds();
    void InitDefaultSimilarityMeasures();

    void InitDefault() {
        void InitDefaultThresholds();
        void InitDefaultSimilarityMeasures();
    }

    void ValidateIndices(config::IndicesType const& indices,
                         SimilaritiesType const& similarity_measures);
    static void ValidateThresholds(config::IndicesType const& indices,
                                   ThresholdsType const& thresholds);

    void ResetState() final;
    void RegisterOptions();
    void VerifyMD();

protected:
    void LoadDataInternal() final;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    MDVerifier();
    bool MDHolds() const;
};

}  // namespace algos::md