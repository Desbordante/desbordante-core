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
    using PliCluster = std::vector<RecordIdentifier>;
    using PositionListIndex = std::vector<PliCluster>;

    // For a record, map partition id to the partition value id of the record in that partition.
    using PartitionValueIdMap = std::vector<PartitionValueId>;
    // Partition id to value id maps for each record.
    using TablePartitionValueIdMaps = std::vector<PartitionValueIdMap>;

private:
    using PositionListIndexes = std::vector<PositionListIndex>;

    PositionListIndexes position_list_indexes_;
    TablePartitionValueIdMaps partition_value_id_maps_;

    PartitionIndex(PositionListIndexes position_list_indexes,
                   TablePartitionValueIdMaps partition_value_id_maps)
        : position_list_indexes_(std::move(position_list_indexes)),
          partition_value_id_maps_(std::move(partition_value_id_maps)) {}

public:
    class PartitionBuilder {
        RecordIdentifier cur_record_id_ = 0;
        PositionListIndex& cur_pli_;
        TablePartitionValueIdMaps::iterator cur_record_iter_;

        PartitionBuilder(PositionListIndex& cur_pli, 
                         TablePartitionValueIdMaps::iterator cur_record_iter)
            : cur_pli_(cur_pli), cur_record_iter_(cur_record_iter) {}

        // Must be called record_number times.
        void AddToCluster(PliCluster& cluster, PartitionValueId partition_value_id) {
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

        PartitionBuilder(PartitionBuilder const&) = delete;
        PartitionBuilder(PartitionBuilder&&) = delete;
        PartitionBuilder& operator=(PartitionBuilder const&) = delete;
        PartitionBuilder& operator=(PartitionBuilder&&) = delete;

        friend class PartitionIndex;
    };

    PartitionIndex(std::size_t record_number, std::size_t partition_number) {
        position_list_indexes_.reserve(partition_number);
        partition_value_id_maps_.assign(record_number, {});
        auto reserve = [partition_number](PartitionValueIdMap& partition_value_id_map) {
            partition_value_id_map.reserve(partition_number);
        };
        std::ranges::for_each(partition_value_id_maps_, reserve);
    }

    // Must be called exactly partition_number times.
    PartitionBuilder NewPartitionBuilder() {
        return {position_list_indexes_.emplace_back(), partition_value_id_maps_.begin()};
    }

    PositionListIndex const& GetPli(model::Index partition_id) const noexcept {
        return position_list_indexes_[partition_id];
    }

    TablePartitionValueIdMaps const& GetPartitionValueIdMaps() const noexcept {
        return partition_value_id_maps_;
    }

    std::size_t GetRecordsNumber() const noexcept {
        return partition_value_id_maps_.size();
    }

    static PartitionIndex GetPermuted(PartitionIndex partition_index,
                                      std::span<model::Index> new_placements) {
        std::size_t record_matches_new = new_placements.size();
        PositionListIndexes plis =
                util::GetPreallocatedVector<PositionListIndex>(record_matches_new);
        for (model::Index i : new_placements) {
            plis.push_back(std::move(partition_index.position_list_indexes_[i]));
        }
        TablePartitionValueIdMaps mapping = util::GetPreallocatedVector<PartitionValueIdMap>(
                partition_index.GetRecordsNumber());
        for (PartitionValueIdMap const& record_clusters :
             partition_index.partition_value_id_maps_) {
            PartitionValueIdMap& new_record_clusters = mapping.emplace_back();
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
