#include "algorithms/ucc_verifier/stats_calculator.h"

#include <algorithm>
#include <numeric>
#include <unordered_map>

#include <easylogging++.h>

namespace algos::ucc_verifier {

void StatsCalculator::PrintStatistics() const {
    if (UCCHolds()) {
        LOG(DEBUG) << "UCC holds.";
    } else {
        LOG(DEBUG) << "UCC does not hold.";
        LOG(DEBUG) << "Number of clusters with errors: " << GetNumErrorClusters();
        LOG(DEBUG) << "Number of rows with errors: " << GetNumErrorRows();
        VisualizeClusters();
    }
}

void StatsCalculator::CalculateStatistics(std::deque<util::PLI::Cluster> clusters) {
    for (auto& cluster : clusters) {
        num_error_rows_ += cluster.size();
        err_clusters_.emplace_back(std::move(cluster));
    }
}

void StatsCalculator::VisualizeClusters() const {
    for (auto const& cluster : err_clusters_) {
    LOG(DEBUG) << "- Tuple: " << GetStringValue(cluster[0]);
    LOG(DEBUG) << "  Size: " << cluster.size();
    std::string RowIndices = "  Row indices: ";
    for (auto i = cluster.cbegin(); i < cluster.cend(); ++i) {
        RowIndices += std::to_string(*i);
        RowIndices += " ";
    }
    LOG(DEBUG) << RowIndices;
    }
}

std::string StatsCalculator::GetStringValue(ClusterIndex row_index) const {
    std::string value;
    for (size_t j = 0; j < column_indices_.size(); ++j) {
        value += GetStringValueByIndex(row_index, column_indices_[j]);
        if (j == /*lhs_indices_*/column_indices_.size() - 1) {
            break;
        }
        value += ", ";
    }
    if (column_indices_.size() > 1) {
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

}  // namespace algos::ucc_verifier