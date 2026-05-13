#pragma once

#include <algorithm>
#include <chrono>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/dd_algorithm.h"
#include "core/algorithms/dd/fastdd/model/differential_dependency.h"
#include "core/algorithms/dd/fastdd/model/pli_shard.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"
#include "core/algorithms/dd/fastdd/util/diff_set_builder.h"
#include "core/algorithms/dd/fastdd/util/differential_function_builder.h"
#include "core/algorithms/dd/fastdd/util/distance_calculator.h"
#include "core/algorithms/dd/fastdd/util/hybrid_evidence_inverter.h"
#include "core/algorithms/dd/fastdd/util/static_bitset.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_index.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/builtin.h"
#include "core/util/logger.h"

namespace algos::dd {

class FastDD final : public DDAlgorithm {
private:
    config::InputTable input_table_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    unsigned num_rows_;
    model::ColumnIndex num_columns_;
    unsigned shard_length_;

    std::vector<model::TypeId> type_ids_;

    config::InputTable difference_table_;
    std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation_;

    std::vector<DifferentialDependency> dds_;

    void RegisterOptions();
    void SetLimits();
    void CheckTypes();
    void ParseDifferenceTable();

    template <BoostDynamicBitsetCompatible Bitset>
    void RunAlgo(DifferentialFunctionBuilder& df_builder,
                 std::shared_ptr<DistanceCalculator> const& distance_calculator,
                 std::vector<PliShard> pli_shards,
                 std::chrono::_V2::system_clock::time_point start_time) {
        DiffSetBuilder<Bitset> diff_set_builder(df_builder, distance_calculator);
        diff_set_builder.BuildDiffSet(std::move(pli_shards));
        DiffSet<Bitset> diff_set = diff_set_builder.GetDiffSet();
        LOG_INFO("Built Diff-Set");
        std::vector<MatchDF<Bitset>> match_dfs = diff_set.GetMatchDFs();
        LOG_INFO("Diff-Set size: {}", match_dfs.size());
        auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - start_time);
        LOG_DEBUG("Current time: {}", elapsed_milliseconds.count());
        std::vector<model::DFConstraint> min_max_dif = diff_set_builder.GetMinMaxDif();
        df_builder.UpdateDFList(min_max_dif);
        std::size_t dif_func_num = df_builder.GetDifFuncNum();
        LOG_INFO("Updated search space size: {}", dif_func_num);

        if (dif_func_num <= 32) {
            RunInverter<StaticBitset<32>, Bitset>(df_builder, match_dfs);
        } else if (dif_func_num <= 64) {
            RunInverter<StaticBitset<64>, Bitset>(df_builder, match_dfs);
        } else if (dif_func_num <= 128) {
            RunInverter<StaticBitset<128>, Bitset>(df_builder, match_dfs);
        } else {
            RunInverter<boost::dynamic_bitset<>, Bitset>(df_builder, match_dfs);
        }
    }

    template <BoostDynamicBitsetCompatible NewBitset, BoostDynamicBitsetCompatible OldBitset>
    void RunInverter(DifferentialFunctionBuilder const& df_builder,
                     std::vector<MatchDF<OldBitset>> const& match_dfs) {
        std::vector<NewBitset> bitsets;
        bitsets.reserve(match_dfs.size());
        std::ranges::transform(
                match_dfs, std::back_inserter(bitsets), [&df_builder](auto const& match_df) {
                    return df_builder.TranslateBitset<OldBitset, NewBitset>(match_df.GetBitset());
                });
        HybridEvidenceInverter<NewBitset> hybrid_evidence_inverter(std::move(bitsets), df_builder);
        LOG_INFO("Built Inverter");

        dds_ = hybrid_evidence_inverter.BuildDDs();
    }

    virtual void ResetStateDD() override {
        dds_.clear();
    }

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    FastDD();

    std::vector<DifferentialDependency> const& GetDDs() const {
        return dds_;
    }
};

}  // namespace algos::dd
