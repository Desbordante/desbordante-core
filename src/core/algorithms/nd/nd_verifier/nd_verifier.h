#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/nd/nd.h"
#include "algorithms/nd/nd_verifier/util/stats_calculator.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/column_layout_relation_data.h"
#include "model/table/column_layout_typed_relation_data.h"

namespace algos::nd_verifier {

/// @brief Algorithm for verifying if ND holds with given weight
class NDVerifier : public Algorithm {
private:
    config::InputTable input_table_;
    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;
    model::WeightType weight_;
    config::EqNullsType is_null_equal_null_;

    using ValueType = std::string;
    using CodeType = size_t;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    util::StatsCalculator<std::string> stats_calculator_;

    void RegisterOptions();
    void ResetState() override;

    std::pair<std::shared_ptr<std::vector<std::string>>, std::shared_ptr<std::vector<size_t>>>
    EncodeMultipleValues(config::IndicesType const& col_idxs) const;
    std::pair<std::shared_ptr<std::vector<std::string>>, std::shared_ptr<std::vector<size_t>>>
    EncodeSingleValues(config::IndexType col_idx) const;
    std::pair<std::shared_ptr<std::vector<std::string>>, std::shared_ptr<std::vector<size_t>>>
    EncodeValues(config::IndicesType const& col_idxs) const;
    void VerifyND();

    void CalculateStats() {
        stats_calculator_.CalculateStats();
    }

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    NDVerifier();
    [[nodiscard]] bool NDHolds();

    [[nodiscard]] auto const& GetHighlights() const {
        return stats_calculator_.GetHighlights();
    }

    [[nodiscard]] auto GetGlobalMinWeight() const {
        return stats_calculator_.GetGlobalMinWeight();
    }

    [[nodiscard]] auto GetRealWeight() const {
        return stats_calculator_.GetRealWeight();
    }

    [[nodiscard]] auto const& GetLhsFrequencies() const {
        return stats_calculator_.GetLhsFrequencies();
    }

    [[nodiscard]] auto const& GetRhsFrequencies() const {
        return stats_calculator_.GetRhsFrequencies();
    }
};

}  // namespace algos::nd_verifier
