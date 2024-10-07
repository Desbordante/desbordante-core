#pragma once

#include "algorithms/algorithm.h"
#include "algorithms/od/fastod/model/canonical_od.h"
#include "config/indices/type.h"
#include "model/table/column_layout_relation_data.h"
#include "partition.h"

namespace algos::od_verifier {

class ODVerifier : public Algorithm {
private:
    using IndicesType = config::IndicesType;
    using IndexType = config::IndexType;
    using DataFrame = fastod::DataFrame;
    using PartitionCache = fastod::PartitionCache;
    using AscCanonicalOD = fastod::AscCanonicalOD;
    using DescCanonicalOD = fastod::DescCanonicalOD;
    using SimpleCanonicalOD = fastod::SimpleCanonicalOD;
    using AttributeSet = fastod::AttributeSet;

    // input data
    config::InputTable input_table_;
    config::EqNullsType is_null_equal_null_;
    IndexType lhs_indicex_;
    IndexType rhs_indicex_;
    IndicesType context_indices_;
    bool ascending_;

    // auxiliary data
    std::shared_ptr<ColumnLayoutRelationData> relation_;
    std::shared_ptr<DataFrame> data_;
    PartitionCache partition_cache_;

    // rows that vioalates ods
    std::vector<int> row_violate_ods_by_swap_;
    std::vector<int> row_violate_ods_by_split_;

    // load input data
    void RegisterOptions();
    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() override;

    // runs the algorithm and measures its time
    unsigned long long ExecuteInternal() override;

    // checks whether OD is violated and finds the rows where it is violated
    template <bool Ascending>
    void VerifyOD() {
        AttributeSet context;

        for (auto column : context_indices_) context.Set(column);

        fastod::ComplexStrippedPartition stripped_partition_swap(
                (partition_cache_.GetStrippedPartition(context, data_)));

        if (stripped_partition_swap.Swap<Ascending>(lhs_indicex_, rhs_indicex_)) {
            ComplaxStrippedPartition part{stripped_partition_swap};
            std::vector<std::pair<int, int>> violates(
                    part.FindViolationsBySwap<Ascending>(lhs_indicex_, rhs_indicex_));

            for (auto position_violate : violates)
                row_violate_ods_by_swap_.push_back(position_violate.second + 1);
        }

        context.Set(lhs_indicex_);
        fastod::ComplexStrippedPartition stripped_partition_split(
                partition_cache_.GetStrippedPartition(context, data_));

        if (stripped_partition_split.Split(rhs_indicex_)) {
            ComplaxStrippedPartition part{stripped_partition_split};
            std::vector<std::pair<int, int>> violates(part.FindViolationsBySplit(rhs_indicex_));

            for (auto position_violate : violates)
                row_violate_ods_by_split_.push_back(position_violate.second + 1);
        }
        std::sort(row_violate_ods_by_split_.begin(), row_violate_ods_by_split_.end());
        std::sort(row_violate_ods_by_swap_.begin(), row_violate_ods_by_swap_.end());
    }

    // reset statistic of violations
    void ResetState() override {
        row_violate_ods_by_swap_.clear();
        row_violate_ods_by_split_.clear();
    }

public:
    // base constructor
    ODVerifier();

    // checks whether the OD has broken
    bool ODHolds() const;

    // Returns the number of rows that violate the OD by split
    size_t GetNumRowsViolateBySplit() const;

    // Returns the number of rows that violate the OD by swap
    size_t GetNumRowsViolateBySwap() const;
};

}  // namespace algos::od_verifier
