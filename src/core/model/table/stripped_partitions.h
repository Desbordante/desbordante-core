#pragma once

#include <vector>

#include "core/model/index.h"
#include "core/model/table/idataset_stream.h"
#include "core/model/table/position_list_index.h"

namespace model {
// This is trying to mimic LegacyColumnLayoutRelationData for now, but without the schema member. We
// should rethink what is being done instead.
class StrippedPartitions {
    std::vector<PositionListIndex> stripped_partitions_;

public:
    // TODO: the current PLIs are woefully overengineered. This should not require a method call.
    StrippedPartitions(std::vector<PositionListIndex> stripped_partitions)
        : stripped_partitions_(std::move(stripped_partitions)) {
        for (PositionListIndex& stripped_partition : stripped_partitions_) {
            stripped_partition.ForceCacheProbingTable();
        }
    }

    [[nodiscard]] size_t GetNumRows() const {
        if (stripped_partitions_.empty()) return 0;

        return stripped_partitions_.front().GetCachedProbingTable()->size();
    }

    std::vector<PositionListIndex> const& GetStrippedPartitions() const noexcept {
        return stripped_partitions_;
    }

    PositionListIndex const& GetStrippedPartition(Index i) const noexcept {
        return stripped_partitions_[i];
    }

    static std::unique_ptr<StrippedPartitions> CreateFrom(IDatasetStream& data_stream);
};
}  // namespace model
