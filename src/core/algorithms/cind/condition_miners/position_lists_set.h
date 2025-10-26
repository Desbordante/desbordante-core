#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include <boost/container_hash/hash.hpp>

class ColumnLayoutRelationData;

template <>
struct std::hash<std::vector<int>> {
    std::size_t operator()(std::vector<int> const& vec) const noexcept {
        return boost::hash_value(vec);
    }
};

namespace model {
class PositionListsSet {
public:
    using Cluster = std::vector<int>;
    using ClusterValue = std::vector<int>;
    using ClusterCollection = std::unordered_map<ClusterValue, Cluster>;

private:
    ClusterCollection clusters_;
    size_t rows_number_;
    size_t relation_size_;

    std::shared_ptr<std::vector<int> const> probing_table_cache_;
    std::vector<ClusterValue> cluster_values_cache_;

public:
    PositionListsSet(ClusterCollection clusters, size_t size, size_t relation_size);

    static std::shared_ptr<PositionListsSet> CreateFor(std::vector<int> const& records,
                                                       size_t relation_size);

    static std::shared_ptr<PositionListsSet> CreateFor(
            std::vector<ClusterCollection::value_type> clusters, size_t relation_size);

    // Calculates PT -- heavy operation
    std::shared_ptr<std::vector<int> const> CalculateAndGetProbingTable();

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
        return rows_number_;
    }

    std::vector<ClusterValue> const& GetClusterValuesCache() const {
        return cluster_values_cache_;
    }

    std::shared_ptr<PositionListsSet> Intersect(PositionListsSet const* that) const;
    std::shared_ptr<PositionListsSet> Probe(PositionListsSet const* that) const;
};

using PLSet = PositionListsSet;

}  // namespace model
