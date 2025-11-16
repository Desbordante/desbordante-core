#include "algorithms/fd/fd_verifier/stats_calculator.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <unordered_map>

#include "util/logger.h"

namespace {

model::CompareResult CompareTypesInCol(model::TypedColumnData const& col,
                                       model::PLI::Cluster::value_type i1,
                                       model::PLI::Cluster::value_type i2) {
    if (col.IsEmpty(i1)) {
        if (col.IsEmpty(i2)) {
            return model::CompareResult::kEqual;
        }
        return model::CompareResult::kLess;
    }
    if (col.IsEmpty(i2)) {
        return model::CompareResult::kGreater;
    }
    if (col.IsNull(i1)) {
        if (col.IsNull(i2)) {
            return model::CompareResult::kEqual;
        }
        return model::CompareResult::kLess;
    }
    if (col.IsNull(i2)) {
        return model::CompareResult::kGreater;
    }

    std::byte const* v1 = col.GetValue(i1);
    std::byte const* v2 = col.GetValue(i2);

    if (col.GetTypeId() != +model::TypeId::kMixed) {
        return col.GetType().Compare(v1, v2);
    }

    auto const& type = static_cast<model::MixedType const&>(col.GetType());
    return model::StringType::Compare(type.ValueToString(v1), type.ValueToString(v2));
}

}  // namespace

namespace algos::fd_verifier {

void StatsCalculator::PrintStatistics() const {
    if (FDHolds()) {
        LOG_DEBUG("FD holds.");
    } else {
        LOG_DEBUG("FD does not hold.");
        LOG_DEBUG("Number of clusters with errors: {}", GetNumErrorClusters());
        LOG_DEBUG("Number of rows with errors: {}", GetNumErrorRows());
        LOG_DEBUG("AFD error threshold: {}", GetError());
        VisualizeHighlights();
    }
}

void StatsCalculator::CalculateStatistics(model::PLI const* lhs_pli, model::PLI const* rhs_pli) {
    std::deque<model::PLI::Cluster> const& lhs_clusters = lhs_pli->GetIndex();
    std::shared_ptr<model::PLI::Cluster const> pt_shared = rhs_pli->CalculateAndGetProbingTable();
    model::PLI::Cluster const& pt = *pt_shared.get();
    size_t num_tuples_conflicting_on_rhs = 0.;

    for (auto& cluster : lhs_clusters) {
        std::unordered_map<ClusterIndex, unsigned> frequencies =
                model::PLI::CreateFrequencies(cluster, pt);
        size_t num_distinct_rhs_values = CalculateNumDistinctRhsValues(frequencies, cluster.size());
        if (num_distinct_rhs_values == 1) {
            continue;
        }
        num_tuples_conflicting_on_rhs +=
                CalculateNumTuplesConflictingOnRhsInCluster(frequencies, cluster.size());
        num_error_rows_ += cluster.size();
        highlights_.emplace_back(std::move(cluster), num_distinct_rhs_values,
                                 CalculateNumMostFrequentRhsValue(frequencies));
    }
    assert(!highlights_.empty());

    size_t num_rows = relation_->GetNumRows();
    error_ = (double)num_tuples_conflicting_on_rhs / (num_rows * num_rows - num_rows);
}

size_t StatsCalculator::CalculateNumMostFrequentRhsValue(
        std::unordered_map<ClusterIndex, unsigned> const& frequencies) {
    if (frequencies.empty()) {
        return 1;
    }
    auto comp = [](auto const& a, auto const& b) { return a.second < b.second; };
    return std::max_element(frequencies.begin(), frequencies.end(), comp)->second;
}

size_t StatsCalculator::CalculateNumTuplesConflictingOnRhsInCluster(
        std::unordered_map<ClusterIndex, unsigned> const& frequencies, size_t cluster_size) {
    size_t num_tuples_conflicting_on_rhs = cluster_size * (cluster_size - 1);
    for (auto const& pair : frequencies) {
        // Frequency can be 1 if lhs cluster intersects with some rhs cluster by one value
        if (pair.second > 1) {
            num_tuples_conflicting_on_rhs -= pair.second * (pair.second - 1);
        }
    }
    return num_tuples_conflicting_on_rhs;
}

size_t StatsCalculator::CalculateNumDistinctRhsValues(
        std::unordered_map<ClusterIndex, unsigned> const& frequencies, size_t cluster_size) {
    size_t num_non_singleton_rhs_values =
            std::accumulate(frequencies.begin(), frequencies.end(), 0U,
                            [](auto a, auto const& b) { return a + b.second; });
    return frequencies.size() + cluster_size - num_non_singleton_rhs_values;
}

void StatsCalculator::VisualizeHighlights() const {
    for (auto const& highlight : highlights_) {
        LOG_DEBUG(
                "- LHS value: {}, Size: {}, Number of different RHS values: {}, "
                "Proportion of most frequent RHS value: {}",
                GetLhsStringValue(highlight.GetCluster()[0]), highlight.GetCluster().size(),
                highlight.GetNumDistinctRhsValues(), highlight.GetMostFrequentRhsValueProportion());
        if (rhs_indices_.size() == 1) {
            for (auto index : highlight.GetCluster()) {
                LOG_DEBUG("{}", GetStringValueByIndex(index, rhs_indices_[0]));
            }
        }
    }
}

std::string StatsCalculator::GetLhsStringValue(ClusterIndex row_index) const {
    std::string value;
    for (size_t j = 0; j < lhs_indices_.size(); ++j) {
        value += GetStringValueByIndex(row_index, lhs_indices_[j]);
        if (j == lhs_indices_.size() - 1) {
            break;
        }
        value += ", ";
    }
    if (lhs_indices_.size() > 1) {
        value.insert(0, "(");
        value.push_back(')');
    }
    return value;
}

std::string StatsCalculator::GetStringValueByIndex(ClusterIndex row_index,
                                                   ClusterIndex col_index) const {
    model::TypedColumnData const& col = typed_relation_->GetColumnData(col_index);
    return col.GetDataAsString(row_index);
}

model::CompareResult StatsCalculator::CompareTypedValues(ClusterIndex i1, ClusterIndex i2) const {
    for (auto index : lhs_indices_) {
        model::CompareResult result =
                CompareTypesInCol(typed_relation_->GetColumnData(index), i1, i2);
        if (result != model::CompareResult::kEqual) {
            return result;
        }
    }
    return model::CompareResult::kEqual;
}

void StatsCalculator::SortHighlights(HighlightCompareFunction const& compare) {
    std::sort(highlights_.begin(), highlights_.end(), compare);
}

auto StatsCalculator::CompareHighlightsByProportionAscending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetMostFrequentRhsValueProportion() < h2.GetMostFrequentRhsValueProportion();
    };
}

auto StatsCalculator::CompareHighlightsByProportionDescending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetMostFrequentRhsValueProportion() > h2.GetMostFrequentRhsValueProportion();
    };
}

auto StatsCalculator::CompareHighlightsByNumAscending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetNumDistinctRhsValues() < h2.GetNumDistinctRhsValues();
    };
}

auto StatsCalculator::CompareHighlightsByNumDescending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetNumDistinctRhsValues() > h2.GetNumDistinctRhsValues();
    };
}

auto StatsCalculator::CompareHighlightsBySizeAscending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetCluster().size() < h2.GetCluster().size();
    };
}

auto StatsCalculator::CompareHighlightsBySizeDescending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetCluster().size() > h2.GetCluster().size();
    };
}

auto StatsCalculator::CompareHighlightsByLhsAscending() const -> HighlightCompareFunction {
    return [this](auto const& h1, auto const& h2) {
        return CompareTypedValues(h1.GetCluster()[0], h2.GetCluster()[0]) ==
               model::CompareResult::kLess;
    };
}

auto StatsCalculator::CompareHighlightsByLhsDescending() const -> HighlightCompareFunction {
    return [this](auto const& h1, auto const& h2) {
        return CompareTypedValues(h1.GetCluster()[0], h2.GetCluster()[0]) ==
               model::CompareResult::kGreater;
    };
}

}  // namespace algos::fd_verifier
