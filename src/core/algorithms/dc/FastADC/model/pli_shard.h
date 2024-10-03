#pragma once

#include <string>
#include <vector>

#include "table/typed_column_data.h"

namespace algos::fastadc {

struct PliShard;

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
class Pli {
public:
    friend PliShard;

    using Cluster = std::vector<size_t>;

    Pli(std::vector<Cluster> raw_clusters, std::vector<size_t> keys,
        std::unordered_map<size_t, size_t> translator);

    size_t Size() const noexcept {
        return keys_.size();
    }

    std::vector<size_t> const& GetKeys() const noexcept {
        return keys_;
    }

    std::vector<Cluster> const& GetClusters() const noexcept {
        return clusters_;
    }

    Cluster const& Get(size_t idx) const noexcept {
        return clusters_[idx];
    }

    size_t GetClusterIdByKey(size_t key) const;

    size_t GetFirstIndexWhereKeyIsLTE(size_t target, size_t l = 0) const;

    std::string ToString() const;

private:
    std::vector<Cluster> clusters_;
    std::vector<size_t> keys_;
    std::unordered_map<size_t, size_t> key_to_cluster_id_map_;

public:
    PliShard const* pli_shard_;
};

/**
 * Shard of PLIs, i.e PLIs for a segment of the dataset.
 *
 * Each PliShard contains multiple PLIs, each corresponding to a column within the shard's row
 * range. This division into shards allows for parallel processing of data.
 */
struct PliShard {
    std::vector<Pli> plis;
    /* The beginning row index of this shard */
    size_t beg;
    /* The end row index of this shard (exclusive) */
    size_t end;

    PliShard(std::vector<Pli> plis, size_t beg, size_t end)
        : plis(std::move(plis)), beg(beg), end(end) {
        assert(beg < end);
        for (auto& pli : this->plis) pli.pli_shard_ = this;
    }

    PliShard(PliShard const&) = delete;
    PliShard& operator=(PliShard const&) = delete;

    PliShard(PliShard&& other) noexcept
        : plis(std::move(other.plis)), beg(other.beg), end(other.end) {
        for (auto& pli : plis) pli.pli_shard_ = this;
    }

    PliShard& operator=(PliShard&& other) noexcept {
        if (this != &other) {
            plis = std::move(other.plis);
            beg = other.beg;
            end = other.end;
            for (auto& pli : plis) pli.pli_shard_ = this;
        }
        return *this;
    }

    size_t Range() const {
        return end - beg;
    }

    std::string ToString() const;
};

class PliShardBuilder {
private:
    size_t shard_length_;
    std::vector<PliShard> pli_shards_;
    Pli BuildPli(std::vector<size_t> const& col_values, bool is_num, size_t beg, size_t end);

public:
    PliShardBuilder(PliShardBuilder const& other) = delete;
    PliShardBuilder& operator=(PliShardBuilder const& other) = delete;
    PliShardBuilder(PliShardBuilder&& other) noexcept = default;
    PliShardBuilder& operator=(PliShardBuilder&& other) noexcept = default;
    ~PliShardBuilder();

    PliShardBuilder(size_t shard_length = 350);

    /**
     * Construct PliShards from the provided table data.
     *
     * Each shard covers a segment of rows defined by `shard_length_`.
     * This method processes the entire table, creating a Pli for each column in each shard.
     */
    void BuildPliShards(std::vector<model::TypedColumnData> const& input);

    std::vector<PliShard> const& GetPliShards() const noexcept {
        return pli_shards_;
    };
};

}  // namespace algos::fastadc
