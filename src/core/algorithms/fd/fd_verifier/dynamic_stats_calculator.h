#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "algorithms/fd/fd_verifier/highlight.h"
#include "config/indices/type.h"
#include "model/table/dynamic_position_list_index.h"
#include "model/table/dynamic_table_data.h"

namespace model {
struct DynamicTableData;
}  // namespace model

namespace algos::fd_verifier {

class DynamicStatsCalculator {
private:
    using ClusterIndex = model::DynPLI::Cluster::value_type;
    using Frequencies = std::unordered_map<ClusterIndex, unsigned>;

    std::shared_ptr<model::DynamicTableData> table_data_;
    config::IndicesType lhs_indices_;
    config::IndicesType rhs_indices_;

    size_t num_error_rows_ = 0;
    long double error_ = 0;
    std::vector<Highlight> highlights_;

    static size_t CalculateNumDistinctRhsValues(Frequencies const& frequencies,
                                                size_t cluster_size);

    static size_t CalculateNumTuplesConflictingOnRhsInCluster(Frequencies const& frequencies,
                                                              size_t cluster_size);

    static size_t CalculateNumMostFrequentRhsValue(Frequencies const& frequencies);

public:
    using HighlightCompareFunction = std::function<bool(Highlight const& h1, Highlight const& h2)>;

    void CalculateStatistics(model::DynPLI const* lhs_pli, model::DynPLI const* rhs_pli);

    void ResetState() {
        highlights_.clear();
        num_error_rows_ = 0;
        error_ = 0;
    }

    bool FDHolds() const {
        return highlights_.empty();
    }

    size_t GetNumErrorClusters() const {
        return highlights_.size();
    }

    size_t GetNumErrorRows() const {
        return num_error_rows_;
    }

    long double GetError() const {
        return error_;
    }

    std::vector<Highlight> const& GetHighlights() const {
        return highlights_;
    }

    void SortHighlights(HighlightCompareFunction const& compare);

    static HighlightCompareFunction CompareHighlightsByProportionAscending();
    static HighlightCompareFunction CompareHighlightsByProportionDescending();
    static HighlightCompareFunction CompareHighlightsByNumAscending();
    static HighlightCompareFunction CompareHighlightsByNumDescending();
    static HighlightCompareFunction CompareHighlightsBySizeAscending();
    static HighlightCompareFunction CompareHighlightsBySizeDescending();

    explicit DynamicStatsCalculator(std::shared_ptr<model::DynamicTableData> table_data,
                                    config::IndicesType lhs_indices,
                                    config::IndicesType rhs_indices)
        : lhs_indices_(std::move(lhs_indices)), rhs_indices_(std::move(rhs_indices)) {
        table_data_ = table_data;
    }
};

}  // namespace algos::fd_verifier
