#pragma once

#include "core/algorithms/cfd/fd_first/model/types.h"

// see algorithms/cfd/fd_first/LICENSE

namespace algos::cfd {

class PartitionUtil {
public:
    static bool IsConstRulePartition(SimpleTIdList const& items, RhsesPair2DList const& rhses);
    static unsigned GetPartitionSupport(SimpleTIdList const& pids,
                                        std::vector<unsigned> const& partitions);
    static unsigned GetPartitionError(SimpleTIdList const& pids, PartitionList const& partitions);
};
}  // namespace algos::cfd
