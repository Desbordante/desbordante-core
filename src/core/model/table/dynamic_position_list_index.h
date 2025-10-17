#pragma once
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "model/table/column.h"

class ColumnLayoutRelationData;

template <>
struct std::hash<std::vector<int>> {
    std::size_t operator()(std::vector<int> const& vec) const noexcept {
        std::size_t secret = vec.size();
        std::size_t hash = (1 << secret) ^ secret;
        for (auto const& el : vec) {
            hash ^= (el << secret) - (hash >> secret);
        }
        return hash;
    }
};

namespace model {
class DynamicPositionListIndex {
public:
    using Cluster = std::vector<int>;
    using ClusterValue = std::vector<int>;
    using ClusterCollection = std::unordered_map<ClusterValue, Cluster>;

private:
    ClusterCollection clusters_;
    size_t valid_records_number_;
    size_t next_record_id_;

    std::shared_ptr<std::vector<int> const> probing_table_cache_;
    mutable std::vector<ClusterValue> cluster_values_cache_;

public:
    DynamicPositionListIndex(ClusterCollection clusters, size_t size, size_t relation_size);

    static std::unique_ptr<DynamicPositionListIndex> CreateFor(
            std::vector<std::vector<int>>& records);

    void UpdateWith(std::vector<std::pair<std::optional<size_t>, ClusterValue>>& inserted_records,
                    std::unordered_set<size_t> deleted_records_ids);

    static std::unordered_map<int, unsigned> CreateFrequencies(
            Cluster const& cluster, std::vector<int> const& probing_table);

    // Calculates PT -- heavy operation
    std::shared_ptr<std::vector<int> const> CalculateAndGetProbingTable() const;

    // Returns cached PT or nullptr, if PT has not been cached yet
    std::shared_ptr<std::vector<int> const> GetCachedProbingTable() const {
        return probing_table_cache_;
    };

    // Calculates PT and cache it
    void ForceCacheProbingTable() {
        probing_table_cache_ = CalculateAndGetProbingTable();
    };

    ClusterCollection const& GetClusters() const noexcept {
        return clusters_;
    };

    /* If you use this method and change index in any way, all other methods will become invalid */
    ClusterCollection& GetClusters() noexcept {
        return clusters_;
    }

    unsigned int GetNumCluster() const {
        return clusters_.size();
    }

    unsigned int GetSize() const {
        return valid_records_number_;
    }

    unsigned int GetRelationSize() const {
        return next_record_id_;
    }

    std::vector<ClusterValue> const& GetClusterValuesCache() const {
        return cluster_values_cache_;
    }

    std::unique_ptr<DynamicPositionListIndex> Intersect(DynamicPositionListIndex const* that) const;
    std::unique_ptr<DynamicPositionListIndex> Probe(DynamicPositionListIndex const* that) const;
    std::string ToString() const;
};

using DynPLI = DynamicPositionListIndex;

}  // namespace model
