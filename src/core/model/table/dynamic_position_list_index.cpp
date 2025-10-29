#include "model/table/dynamic_position_list_index.h"

#include <cassert>
#include <memory>
#include <utility>

namespace {
// O(cluster)
void ProcessDeleteToCluster(model::DynamicPositionListIndex::Cluster& cluster,
                            std::unordered_set<size_t>& deleted_records_ids) {
    auto records_pred = [&](size_t element) {
        return static_cast<bool>(deleted_records_ids.erase(element));
    };
    std::erase_if(cluster, records_pred);
}

}  // namespace

namespace model {

DynamicPositionListIndex::DynamicPositionListIndex(ClusterCollection clusters, size_t size,
                                                   size_t relation_size)
    : clusters_(std::move(clusters)),
      valid_records_number_(size),
      next_record_id_(relation_size),
      probing_table_cache_() {
    ForceCacheProbingTable();
}

std::unique_ptr<DynamicPositionListIndex> DynamicPositionListIndex::CreateFor(
        std::vector<ClusterValue>& records) {
    ClusterCollection clusters;
    for (size_t record_id = 0; record_id < records.size(); ++record_id) {
        ClusterValue const& value = records[record_id];
        clusters[value].push_back(record_id);
    }

    return std::make_unique<DynamicPositionListIndex>(std::move(clusters), records.size(),
                                                      records.size());
}

void DynamicPositionListIndex::UpdateWith(
        std::vector<std::pair<std::optional<size_t>, ClusterValue>>& inserted_records,
        std::unordered_set<size_t> deleted_records_ids) {
    if (inserted_records.empty() && deleted_records_ids.empty()) {
        return;
    }
    valid_records_number_ -= deleted_records_ids.size();
    for (auto it = clusters_.begin(); it != clusters_.end();) {
        auto& [cluster_value, cluster] = *it;
        ProcessDeleteToCluster(cluster, deleted_records_ids);
        if (cluster.empty()) {
            it = clusters_.erase(it);
        } else {
            ++it;
        }
        if (deleted_records_ids.empty()) break;
    }
    for (auto const& [encoded_value_opt, cluster_value] : inserted_records) {
        if (encoded_value_opt.has_value()) {
            clusters_[cluster_value].emplace_back(*encoded_value_opt);
        } else {
            clusters_[cluster_value].emplace_back(next_record_id_++);
            ++valid_records_number_;
        }
    }

    ForceCacheProbingTable();
}

std::unordered_map<int, unsigned> DynamicPositionListIndex::CreateFrequencies(
        Cluster const& cluster, std::vector<int> const& probing_table) {
    std::unordered_map<int, unsigned> frequencies;

    for (int index : cluster) {
        frequencies[probing_table[index]]++;
    }

    return frequencies;
}

std::shared_ptr<std::vector<int> const> DynamicPositionListIndex::CalculateAndGetProbingTable()
        const {
    // cluster values cache: used in the Probe() method
    cluster_values_cache_.clear();
    std::vector<int> probing_table = std::vector<int>(next_record_id_);
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

std::unique_ptr<DynamicPositionListIndex> DynamicPositionListIndex::Intersect(
        DynamicPositionListIndex const* that) const {
    assert(this->next_record_id_ == that->next_record_id_);

    if (this->valid_records_number_ > that->valid_records_number_) {
        return that->Probe(this);
    } else {
        return this->Probe(that);
    }
}

std::unique_ptr<DynamicPositionListIndex> DynamicPositionListIndex::Probe(
        DynamicPositionListIndex const* that) const {
    std::shared_ptr<std::vector<int> const> probing_table = that->GetCachedProbingTable();
    std::vector<ClusterValue> const& cluster_values = that->GetClusterValuesCache();
    assert(this->next_record_id_ == probing_table->size());
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

    return std::make_unique<DynamicPositionListIndex>(std::move(new_clusters), new_size,
                                                      next_record_id_);
}

std::string DynamicPositionListIndex::ToString() const {
    std::string res = "[";
    for (auto& cluster : clusters_) {
        res.push_back('[');
        for (int v : cluster.second) {
            res.append(std::to_string(v) + ",");
        }
        if (res.find(',') != std::string::npos) res.erase(res.find_last_of(','));
        res.push_back(']');
        res.push_back(',');
    }
    if (res.find(',') != std::string::npos) res.erase(res.find_last_of(','));
    res.push_back(']');
    return res;
}

}  // namespace model
