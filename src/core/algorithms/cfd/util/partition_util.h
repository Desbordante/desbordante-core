#pragma once

#include "algorithms/cfd/model/cfd_types.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class PartitionUtil {
public:
    static bool IsConstRulePartition(SimpleTIdList const& items, RhsesPair2DList const& rhses);
    static unsigned GetPartitionSupport(SimpleTIdList const& pids,
                                        std::vector<unsigned> const& partitions);
    static unsigned GetPartitionError(SimpleTIdList const& pids, PartitionList const& partitions);
};
}  // namespace algos::cfd
