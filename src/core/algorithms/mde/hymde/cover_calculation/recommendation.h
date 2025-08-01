#pragma once

#include <vector>

#include "algorithms/mde/hymde/record_match_indexes/partition_index.h"

namespace algos::hymde::cover_calculation {
struct Recommendation {
    std::vector<record_match_indexes::PartitionIndex::PartitionValueIdMap const*>
            left_record_pvid_map_ptrs;
    record_match_indexes::PartitionIndex::PartitionValueIdMap const* right_record_pvid_map_ptr;
};

using LhsGroupedRecommendations = std::vector<Recommendation>;
}  // namespace algos::hymde::cover_calculation
