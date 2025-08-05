#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dc/FastADC/misc/misc.h"
#include "dc/FastADC/providers/index_provider.h"
#include "table/typed_column_data.h"

namespace algos::fastadc {

class PliShard;

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

    bool TryGetClusterIdByKey(size_t key, size_t& cluster_id) const;

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
class PliShard {
    std::vector<Pli> plis_;
    /* The beginning row index of this shard */
    size_t beg_;
    /* The end row index of this shard (exclusive) */
    size_t end_;

    void Swap(PliShard& other) noexcept {
        std::swap(plis_, other.plis_);
        std::swap(beg_, other.beg_);
        std::swap(end_, other.end_);

        for (auto& pli : plis_) {
            pli.pli_shard_ = this;
        }

        for (auto& pli : other.plis_) {
            pli.pli_shard_ = &other;
        }
    }

public:
    PliShard(std::vector<Pli> plis, size_t beg, size_t end)
        : plis_(std::move(plis)), beg_(beg), end_(end) {
        assert(beg < end);
        for (auto& pli : this->plis_) pli.pli_shard_ = this;
    }

    PliShard(PliShard const&) = delete;
    PliShard& operator=(PliShard const&) = delete;

    PliShard(PliShard&& other) noexcept : plis_(), beg_(0), end_(0) {
        Swap(other);
    }

    PliShard& operator=(PliShard other) noexcept {
        Swap(other);
        return *this;
    }

    std::vector<Pli> const& Plis() const noexcept {
        return plis_;
    }

    size_t Beg() const noexcept {
        return beg_;
    }

    size_t Range() const noexcept {
        return end_ - beg_;
    }

    std::string ToString() const;
};

class PliShardBuilder {
private:
    size_t shard_length_;
    Pli BuildPli(std::vector<size_t> const& col_values, bool is_num, size_t beg, size_t end);

    IntIndexProvider* int_provider_;
    DoubleIndexProvider* double_provider_;
    StringIndexProvider* string_provider_;

    template <typename T>
    size_t AddValueToHash(model::TypedColumnData const& column, size_t row) {
        if constexpr (std::is_same_v<T, int64_t>)
            return int_provider_->GetIndex(GetValue<int64_t>(column, row));
        else if constexpr (std::is_same_v<T, double>)
            return double_provider_->GetIndex(GetValue<double>(column, row));
        else if constexpr (std::is_same_v<T, std::string>)
            return string_provider_->GetIndex(GetValue<std::string>(column, row));
        else
            static_assert(details::DependentFalse<T>::value,
                          "PliShardBuilder does not unsupport that type");
    }

    template <typename T>
    void ColumnToHashTyped(std::vector<size_t>& hashed_column,
                           model::TypedColumnData const& column) {
        for (size_t i = 0; i < column.GetNumRows(); i++) {
            hashed_column[i] = AddValueToHash<T>(column, i);
        }
    }

    std::vector<size_t> ColumnToHash(model::TypedColumnData const& column);

    void AddTableToHashAndSortProviders(std::vector<model::TypedColumnData> const& input);

public:
    std::vector<PliShard> pli_shards;

    PliShardBuilder(PliShardBuilder const& other) = delete;
    PliShardBuilder& operator=(PliShardBuilder const& other) = delete;
    PliShardBuilder(PliShardBuilder&& other) noexcept = default;
    PliShardBuilder& operator=(PliShardBuilder&& other) noexcept = default;

    PliShardBuilder(IntIndexProvider* int_provider, DoubleIndexProvider* double_provider,
                    StringIndexProvider* string_provider, size_t shard_length = 350);

    /**
     * Construct PliShards from the provided table data.
     *
     * Each shard covers a segment of rows defined by `shard_length_`.
     * This method processes the entire table, creating a Pli for each column in each shard.
     */
    void BuildPliShards(std::vector<model::TypedColumnData> const& input);
};

}  // namespace algos::fastadc
