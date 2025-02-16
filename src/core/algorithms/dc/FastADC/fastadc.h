#pragma once

#include <memory>
#include <vector>

#include "algorithms/algorithm.h"
#include "dc/FastADC/providers/predicate_provider.h"
#include "dc/FastADC/util/denial_constraint_set.h"
#include "model/denial_constraint.h"
#include "table/column_layout_typed_relation_data.h"
#include "tabular_data/input_table_type.h"

namespace algos::dc {

using namespace fastadc;

class FastADC : public Algorithm {
private:
    unsigned shard_length_;
    bool allow_cross_columns_;
    double minimum_shared_value_;
    double comparable_threshold_;
    double evidence_threshold_;

    config::InputTable input_table_;
    std::unique_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    PredicateIndexProvider pred_index_provider_;
    PredicateProvider pred_provider_;
    IntIndexProvider int_prov_;
    DoubleIndexProvider double_prov_;
    StringIndexProvider string_prov_;
    DenialConstraintSet dcs_;

    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() override;

    void SetLimits();
    void CheckTypes();
    void PrintResults();

    void ResetState() final {
        pred_index_provider_.Clear();
        pred_provider_.Clear();
        int_prov_.Clear();
        double_prov_.Clear();
        string_prov_.Clear();
        dcs_.Clear();
    }

    unsigned long long ExecuteInternal() final;

    void RegisterOptions();

public:
    FastADC();

    std::vector<DenialConstraint> const& GetDCs() const;
};

}  // namespace algos::dc
