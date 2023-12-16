#pragma once

#include "algorithms/cfd/model/cfd_types.h"
#include "algorithms/cfd/model/partition_tidlist.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class TIdUtil {
public:
    static int Support(PartitionTIdList const& tids);
    static unsigned Hash(PartitionTIdList const& tids);
    static int Support(SimpleTIdList const& tids);
    static unsigned Hash(SimpleTIdList const& tids);
};
}  // namespace algos::cfd
