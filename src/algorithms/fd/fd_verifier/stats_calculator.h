#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "algorithms/fd/fd_verifier/highlight.h"
#include "model/column_layout_relation_data.h"
#include "model/column_layout_typed_relation_data.h"
#include "util/config/indices/type.h"

namespace algos::fd_verifier {

class StatsCalculator {
private:
    using ClusterIndex = util::PLI::Cluster::value_type;

    std::shared_ptr<ColumnLayoutRelationData> relation_;
    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;

    util::config::IndicesType lhs_indices_;
    util::config::IndicesType rhs_indices_;

    size_t num_error_rows_ = 0;
    long double error_ = 0;
    std::vector<Highlight> highlights_;

    void VisualizeHighlights() const;
    std::string GetLhsStringValue(ClusterIndex row_index) const;
    std::string GetStringValueByIndex(ClusterIndex row_index, ClusterIndex col_index) const;

    static size_t CalculateNumDistinctRhsValues(
            std::unordered_map<ClusterIndex, unsigned> const& frequencies, size_t cluster_size);

    static size_t CalculateNumTuplesConflictingOnRhsInCluster(
            std::unordered_map<ClusterIndex, unsigned> const& frequencies, size_t cluster_size);

    static size_t CalculateNumMostFrequentRhsValue(
            std::unordered_map<ClusterIndex, unsigned> const& frequencies);

    model::CompareResult CompareTypedValues(ClusterIndex i1, ClusterIndex i2) const;

public:
    using HighlightCompareFunction = std::function<bool(Highlight const& h1, Highlight const& h2)>;

    void CalculateStatistics(util::PLI const* lhs_pli, util::PLI const* rhs_pli);

    void PrintStatistics() const;

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
    HighlightCompareFunction CompareHighlightsByLhsAscending() const;
    HighlightCompareFunction CompareHighlightsByLhsDescending() const;

    explicit StatsCalculator(std::shared_ptr<ColumnLayoutRelationData> relation,
                             std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation,
                             util::config::IndicesType lhs_indices,
                             util::config::IndicesType rhs_indices)
        : relation_(std::move(relation)),
          typed_relation_(std::move(typed_relation)),
          lhs_indices_(std::move(lhs_indices)),
          rhs_indices_(std::move(rhs_indices)) {}
};

}  // namespace algos::fd_verifier
