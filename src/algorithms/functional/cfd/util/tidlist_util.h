#pragma once

#include "functional/cfd/structures/cfd_types.h"
#include "functional/cfd/structures/partition_tidlist.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class TIdUtil {
public:
    static int Support(const PartitionTIdList& tids);
    static unsigned Hash(const PartitionTIdList& tids);
    static int Support(const SimpleTIdList& tids);
    static unsigned Hash(const SimpleTIdList& tids);
};
}  // namespace algos::cfd
