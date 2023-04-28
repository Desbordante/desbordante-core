#pragma once

#include "algorithms/cfd/structures/partition_tidlist.h"

namespace algos::cfd {

class PartitionTIdListUtil {
public:
    static std::vector<PartitionTIdList> ConstructIntersection(
            PartitionTIdList const& lhs, const std::vector<const PartitionTIdList*>& rhses);
};
}  // namespace algos::cfd
