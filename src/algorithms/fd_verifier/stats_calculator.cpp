#include "algorithms/fd_verifier/stats_calculator.h"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <unordered_map>

#include <easylogging++.h>

namespace {

model::CompareResult CompareTypesInCol(model::TypedColumnData const& col,
                                       util::PLI::Cluster::value_type i1,
                                       util::PLI::Cluster::value_type i2) {
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
        LOG(DEBUG) << "FD holds.";
    } else {
        LOG(DEBUG) << "FD does not hold.";
        LOG(DEBUG) << "Number of clusters with errors: " << GetNumErrorClusters();
        LOG(DEBUG) << "Number of rows with errors: " << GetNumErrorRows();
        LOG(DEBUG) << "AFD error threshold: " << GetError();
        VisualizeHighlights();
    }
}

void StatsCalculator::CalculateStatistics(std::deque<util::PLI::Cluster> clusters) {
    auto const& pt = relation_->GetColumnData(rhs_index_).GetProbingTable();
    size_t num_tuples_conflicting_on_rhs = 0.;

    for (auto& cluster : clusters) {
        std::unordered_map<ClusterIndex, unsigned> frequencies =
                util::PLI::CreateFrequencies(cluster, pt);
        size_t num_different_rhs_values =
                CalculateNumDifferentRhsValues(frequencies, cluster.size());
        if (num_different_rhs_values == 1) {
            continue;
        }
        num_tuples_conflicting_on_rhs +=
                CalculateNumTuplesConflictingOnRhsInCluster(frequencies, cluster.size());
        num_error_rows_ += cluster.size();
        highlights_.emplace_back(std::move(cluster), num_different_rhs_values,
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

size_t StatsCalculator::CalculateNumDifferentRhsValues(
        std::unordered_map<ClusterIndex, unsigned> const& frequencies, size_t cluster_size) {
    size_t num_non_singleton_rhs_values =
            std::accumulate(frequencies.begin(), frequencies.end(), 0U,
                            [](auto a, auto const& b) { return a + b.second; });
    return frequencies.size() + cluster_size - num_non_singleton_rhs_values;
}

void StatsCalculator::VisualizeHighlights() const {
    for (auto const& highlight : highlights_) {
        LOG(DEBUG) << "- LHS value: " << GetLhsStringValue(highlight.GetCluster()[0])
                   << ", Size: " << highlight.GetCluster().size()
                   << ", Number of different RHS values: " << highlight.GetNumDifferentRhsValues()
                   << ", Proportion of most frequent RHS value: "
                   << highlight.GetMostFrequentValueProportion();
        for (auto index : highlight.GetCluster()) {
            LOG(DEBUG) << GetStringValueByIndex(index, rhs_index_);
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
        return h1.GetMostFrequentValueProportion() < h2.GetMostFrequentValueProportion();
    };
}

auto StatsCalculator::CompareHighlightsByProportionDescending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetMostFrequentValueProportion() > h2.GetMostFrequentValueProportion();
    };
}

auto StatsCalculator::CompareHighlightsByNumAscending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetNumDifferentRhsValues() < h2.GetNumDifferentRhsValues();
    };
}

auto StatsCalculator::CompareHighlightsByNumDescending() -> HighlightCompareFunction {
    return [](auto const& h1, auto const& h2) {
        return h1.GetNumDifferentRhsValues() > h2.GetNumDifferentRhsValues();
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
