#pragma once

#include "algorithms/algorithm.h"
#include "algorithms/nd/nd.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_relation_data.h"

namespace algos::nd_verifier {

/// @brief Algorithm for verifying if ND holds with given weight
class NDVerifier : Algorithm {
private:
    config::InputTable input_table_;
    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;
    WeightType weight_;
    config::EqNullsType is_null_equal_null_;

    using ValueType = std::string;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    StatsCalculator<ValueType> stats_calculator_;

    void RegisterOptions();

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    NDVerifier();
    bool NDHolds() const;
};

}  // namespace algos::nd_verifier
