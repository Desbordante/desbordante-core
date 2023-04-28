#pragma once

#include "algorithms/cfd/structures/cfd_types.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class PartitionUtil {
public:
    static bool IsConstRulePartition(const SimpleTIdList& items, const RhsesPair2DList& rhses);
    static unsigned GetPartitionSupport(const SimpleTIdList& pids,
                                        const std::vector<unsigned>& partitions);
    static unsigned GetPartitionError(const SimpleTIdList& pids, const PartitionList& partitions);
};
}  // namespace algos::cfd
