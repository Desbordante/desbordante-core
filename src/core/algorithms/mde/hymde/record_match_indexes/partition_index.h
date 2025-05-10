#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_identifier.h"
#include "model/index.h"
#include "util/get_preallocated_vector.h"

namespace algos::hymde::record_match_indexes {
class PartitionIndex {
public:
    using RecordCluster = std::vector<RecordIdentifier>;
    using PositionListIndex = std::vector<RecordCluster>;

    using Clusters = std::vector<PartitionValueId>;
    using RecordClustersMapping = std::vector<Clusters>;

private:
    using PositionListIndexes = std::vector<PositionListIndex>;

    PositionListIndexes position_list_indexes_;
    RecordClustersMapping record_clusters_mapping_;

    PartitionIndex(PositionListIndexes position_list_indexes,
                   RecordClustersMapping record_clusters_mapping)
        : position_list_indexes_(std::move(position_list_indexes)),
          record_clusters_mapping_(std::move(record_clusters_mapping)) {}

public:
    class Adder {
        RecordIdentifier cur_record_id_ = 0;
        PositionListIndex& cur_pli_;
        RecordClustersMapping::iterator cur_record_iter_;

        Adder(PositionListIndex& cur_pli, RecordClustersMapping::iterator cur_record_iter)
            : cur_pli_(cur_pli), cur_record_iter_(cur_record_iter) {}

        // Must be called record_number times.
        void AddToCluster(RecordCluster& cluster, PartitionValueId partition_value_id) {
            cluster.push_back(cur_record_id_);
            cur_record_iter_->push_back(partition_value_id);
            ++cur_record_iter_;
            ++cur_record_id_;
        }

    public:
        // Expected scenario: user keeps some sort of value to index mapping.
        void AddToCluster(PartitionValueId partition_value_id) {
            assert(partition_value_id < cur_pli_.size());
            AddToCluster(cur_pli_[partition_value_id], partition_value_id);
        }

        void AddToNewCluster(PartitionValueId partition_value_id) {
            assert(partition_value_id == cur_pli_.size());
            AddToCluster(cur_pli_.emplace_back(), partition_value_id);
        }

        // Should become unneeded if upper set indexes are replaced.
        PositionListIndex const& GetCurrentPli() const noexcept {
            return cur_pli_;
        }

        Adder(Adder const&) = delete;
        Adder(Adder&&) = delete;
        Adder& operator=(Adder const&) = delete;
        Adder& operator=(Adder&&) = delete;

        friend class PartitionIndex;
    };

    PartitionIndex(std::size_t record_number, std::size_t partition_number) {
        position_list_indexes_.reserve(partition_number);
        record_clusters_mapping_.assign(record_number, {});
        auto res = [partition_number](Clusters& clusters) { clusters.reserve(partition_number); };
        std::ranges::for_each(record_clusters_mapping_, res);
    }

    // Must be called exactly partition_number times.
    Adder NewPartitionAdder() {
        return {position_list_indexes_.emplace_back(), record_clusters_mapping_.begin()};
    }

    PositionListIndex const& GetPli(model::Index record_match_index) const noexcept {
        return position_list_indexes_[record_match_index];
    }

    Clusters const& GetRecordClusters(RecordIdentifier record_id) const noexcept {
        return record_clusters_mapping_[record_id];
    }

    RecordClustersMapping const& GetClustersMapping() const noexcept {
        return record_clusters_mapping_;
    }

    std::size_t GetRecordsNumber() const noexcept {
        return record_clusters_mapping_.size();
    }

    std::size_t GetRecordMatchNumber() const noexcept {
        return position_list_indexes_.size();
    }

    static PartitionIndex GetPermuted(PartitionIndex partition_index,
                                      std::span<model::Index> new_placements) {
        std::size_t record_matches_new = new_placements.size();
        PositionListIndexes plis =
                util::GetPreallocatedVector<PositionListIndex>(record_matches_new);
        for (model::Index i : new_placements) {
            plis.push_back(std::move(partition_index.position_list_indexes_[i]));
        }
        RecordClustersMapping mapping =
                util::GetPreallocatedVector<Clusters>(partition_index.GetRecordsNumber());
        for (Clusters const& record_clusters : partition_index.record_clusters_mapping_) {
            Clusters& new_record_clusters = mapping.emplace_back();
            new_record_clusters.reserve(record_matches_new);
            for (model::Index i : new_placements) {
                new_record_clusters.push_back(record_clusters[i]);
            }
        }
        return {std::move(plis), std::move(mapping)};
    }
};

class DataPartitionIndex {
    PartitionIndex left_;
    PartitionIndex right_;

public:
    DataPartitionIndex(PartitionIndex left, PartitionIndex right)
        : left_(std::move(left)), right_(std::move(right)) {}

    PartitionIndex const& GetLeft() const noexcept {
        return left_;
    }

    PartitionIndex const& GetRight() const noexcept {
        return right_;
    }

    std::size_t GetPairsNumber() const noexcept {
        return left_.GetRecordsNumber() * right_.GetRecordsNumber();
    }
};
}  // namespace algos::hymde::record_match_indexes
