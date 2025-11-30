#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/nd/nd.h"
#include "core/algorithms/nd/nd_verifier/util/highlight.h"
#include "core/algorithms/nd/nd_verifier/util/stats_calculator.h"
#include "core/algorithms/nd/nd_verifier/util/value_combination.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/indices/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/builtin.h"

namespace algos::nd_verifier {

/// @brief Algorithm for verifying if ND holds with given weight
class NDVerifier : public Algorithm {
private:
    using CombinedValuesType = std::pair<std::shared_ptr<std::vector<util::ValueCombination>>,
                                         std::shared_ptr<std::vector<size_t>>>;

    config::InputTable input_table_;
    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;
    model::WeightType weight_;
    config::EqNullsType is_null_equal_null_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    util::StatsCalculator stats_calculator_;

    void RegisterOptions();
    void ResetState() override;
    void VerifyND();

    CombinedValuesType CombineValues(config::IndicesType const& col_idxs) const;

    void CalculateStats() {
        stats_calculator_.CalculateStats();
    }

    void AddVCToValues(std::shared_ptr<std::vector<util::ValueCombination>> values,
                       std::shared_ptr<std::vector<size_t>> row,
                       std::vector<std::pair<model::TypeId, std::byte const*>> const& typed_data,
                       bool is_null) const;

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    NDVerifier();
    [[nodiscard]] bool NDHolds() const;
    [[nodiscard]] std::vector<util::Highlight> const& GetHighlights() const;
    [[nodiscard]] model::WeightType GetGlobalMinWeight() const;
    [[nodiscard]] model::WeightType GetRealWeight() const;
    [[nodiscard]] std::unordered_map<std::string, size_t> GetLhsFrequencies() const;
    [[nodiscard]] std::unordered_map<std::string, size_t> GetRhsFrequencies() const;
};

}  // namespace algos::nd_verifier
