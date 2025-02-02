#include "position_lists_set.h"

#include <memory>
#include <utility>

#include <boost/dynamic_bitset.hpp>
#include <easylogging++.h>

namespace model {

PositionListsSet::PositionListsSet(ClusterCollection clusters, size_t size)
    : clusters_(std::move(clusters)), rows_number_(size), probing_table_cache_() {
    ForceCacheProbingTable();
}

std::unique_ptr<PositionListsSet> PositionListsSet::CreateFor(std::vector<ClusterValue> const& records) {
    ClusterCollection clusters;
    for (size_t record_id = 0; record_id < records.size(); ++record_id) {
        ClusterValue const& value = records[record_id];
        clusters[value].push_back(record_id);
    }
    return std::make_unique<PositionListsSet>(std::move(clusters), records.size());
}

std::unique_ptr<PositionListsSet> PositionListsSet::CreateFor(std::vector<ClusterValue> records) {
    ClusterCollection clusters;
    for (size_t record_id = 0; record_id < records.size(); ++record_id) {
        ClusterValue const& value = records[record_id];
        clusters[value].push_back(record_id);
    }
    return std::make_unique<PositionListsSet>(std::move(clusters), records.size());
}

std::unique_ptr<PositionListsSet> PositionListsSet::CreateFor(std::vector<ClusterCollection::value_type> &&records) {
    ClusterCollection clusters;
    for (auto &record : records) {
        clusters.emplace(record);
    }
    return std::make_unique<PositionListsSet>(std::move(clusters), records.size());
}

std::shared_ptr<std::vector<int> const> PositionListsSet::CalculateAndGetProbingTable() {
    cluster_values_cache_.clear();
    std::vector<int> probing_table = std::vector<int>(GetSize());
    int next_cluster_id = 0;
    for (auto& [value, cluster] : clusters_) {
        int value_id = next_cluster_id++;
        for (int position : cluster) {
            probing_table[position] = value_id;
        }
        cluster_values_cache_.emplace_back(value);
    }

    return std::make_shared<std::vector<int>>(probing_table);
}

std::unique_ptr<PositionListsSet> PositionListsSet::Intersect(PositionListsSet const* that) const {
    if (this->rows_number_ > that->rows_number_) {
        return that->Probe(this);
    } else {
        return this->Probe(that);
    }
}

std::unique_ptr<PositionListsSet> PositionListsSet::Probe(PositionListsSet const* that) const {
    std::shared_ptr<std::vector<int> const> probing_table = that->GetCachedProbingTable();
    std::vector<ClusterValue> const& cluster_values = that->GetClusterValuesCache();
    ClusterCollection new_clusters;
    unsigned int new_size = 0;

    std::unordered_map<int, std::pair<ClusterValue, Cluster>> partial_clusters;

    for (auto const& [value, cluster] : clusters_) {
        for (int position : cluster) {
            int probing_table_value_id = (*probing_table)[position];
            auto partial_cluster = partial_clusters.find(probing_table_value_id);
            if (partial_cluster != partial_clusters.end()) {
                auto& [key, positions] = partial_cluster->second;
                positions.emplace_back(position);
            } else {
                ClusterValue key = value;
                auto& cluster_value = cluster_values[probing_table_value_id];
                key.insert(key.end(), cluster_value.begin(), cluster_value.end());
                partial_clusters[probing_table_value_id] = {std::move(key), {position}};
            }
        }

        for (auto& [_, new_cluster] : partial_clusters) {
            new_size += new_cluster.second.size();
            new_clusters.emplace(std::move(new_cluster));
        }
        partial_clusters.clear();
    }

    return std::make_unique<PositionListsSet>(std::move(new_clusters), new_size);
}
}  // namespace model
