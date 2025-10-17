#pragma once

#include <vector>

#include "algorithms/cfd/model/partition_tidlist.h"

namespace algos::cfd {

class PartitionTIdListUtil {
public:
    static std::vector<PartitionTIdList> ConstructIntersection(
            PartitionTIdList const& lhs, std::vector<PartitionTIdList const*> const& rhses);
};
}  // namespace algos::cfd
