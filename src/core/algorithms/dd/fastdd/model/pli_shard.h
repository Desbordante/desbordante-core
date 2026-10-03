#pragma once

#include <assert.h>
#include <cstddef>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/algorithms/dd/fastdd/providers/index_provider.h"
#include "core/model/table/tuple_index.h"
#include "core/model/table/typed_column_data.h"
#include "core/model/types/type.h"

namespace algos::dd {
/**
 * Position List Index (PLI) for a specific column segment.
 *
 * A PLI contains:
 * - A list of unique keys (hashed values from the column).
 * - A list of clusters, where each cluster is a group of row indices sharing the same key.
 * - A mapping from each key to its corresponding cluster ID.
 *
 * Clusters are indexed by the keys' cluster IDs, allowing direct retrieval of rows
 * associated with a specific hashed value.
 */
using Cluster = std::vector<model::TupleIndex>;
using ClusterIndex = std::size_t;

class Pli {
private:
    std::vector<Cluster> clusters_;
    std::vector<std::size_t> keys_;
    std::unordered_map<std::size_t, ClusterIndex> key_to_cluster_id_map_;
    std::vector<ClusterIndex> inverted_index_;

public:
    Pli(std::vector<Cluster> raw_clusters, std::vector<std::size_t> keys,
        std::unordered_map<std::size_t, std::size_t> translator);

    std::size_t Size() const noexcept {
        return keys_.size();
    }

    std::vector<size_t> const& GetKeys() const noexcept {
        return keys_;
    }

    std::vector<Cluster> const& GetClusters() const noexcept {
        return clusters_;
    }

    std::vector<ClusterIndex> const& GetInvertedIndex() const noexcept {
        return inverted_index_;
    }

    Cluster const& GetCluster(std::size_t idx) const noexcept {
        return clusters_[idx];
    }

    ClusterIndex GetClusterIdByRow(model::TupleIndex index) const noexcept {
        return inverted_index_[index];
    }

    bool TryGetClusterIdByKey(std::size_t key, ClusterIndex& cluster_id) const;
    ClusterIndex GetClusterIdByKey(std::size_t key) const;
    std::size_t GetFirstIndexWhereKeyIsLTE(std::size_t target, std::size_t l = 0) const;

    std::string ToString() const;
};

/**
 * Shard of PLIs, i.e PLIs for a segment of the dataset.
 *
 * Each PliShard contains multiple PLIs, each corresponding to a column within the shard's row
 * range. This division into shards allows for parallel processing of data.
 */
class PliShard {
    std::vector<Pli> plis_;
    /* The beginning row index of this shard */
    std::size_t beg_;
    /* The end row index of this shard (exclusive) */
    std::size_t end_;

public:
    PliShard(std::vector<Pli> plis, std::size_t beg, std::size_t end)
        : plis_(std::move(plis)), beg_(beg), end_(end) {
        assert(beg < end);
    }

    PliShard(PliShard const&) = delete;
    PliShard& operator=(PliShard const&) = delete;

    PliShard(PliShard&& other) noexcept = default;

    PliShard& operator=(PliShard&& other) noexcept = default;

    std::vector<Pli> const& Plis() const noexcept {
        return plis_;
    }

    std::size_t Beg() const noexcept {
        return beg_;
    }

    std::size_t Range() const noexcept {
        return end_ - beg_;
    }

    std::string ToString() const;
};

class PliShardBuilder {
private:
    std::size_t shard_length_;
    std::vector<IndexProvider<std::byte const*, model::Type::Hasher, model::Type::Equal>>
            providers_;

    Pli BuildPli(std::vector<std::size_t> const& col_values, bool is_distance_ordered,
                 std::size_t beg, std::size_t end);
    std::vector<std::size_t> ColumnToHash(model::TypedColumnData const& column,
                                          model::ColumnIndex column_index);
    void AddTableToHashAndSortProviders(std::vector<model::TypedColumnData> const& input);

public:
    PliShardBuilder(PliShardBuilder const& other) = delete;
    PliShardBuilder& operator=(PliShardBuilder const& other) = delete;
    PliShardBuilder(PliShardBuilder&& other) noexcept = default;
    PliShardBuilder& operator=(PliShardBuilder&& other) noexcept = default;

    PliShardBuilder(std::size_t shard_length = 10000) : shard_length_(shard_length) {}

    /**
     * Construct PliShards from the provided table data.
     *
     * Each shard covers a segment of rows defined by `shard_length_`.
     * This method processes the entire table, creating a Pli for each column in each shard.
     */
    std::vector<PliShard> BuildPliShards(std::vector<model::TypedColumnData> const& input);
};

}  // namespace algos::dd
