#include "tidlist_util.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

int TIdUtil::Support(const PartitionTIdList& tids) {
    if (!tids.tids.empty()) {
        return tids.tids.size() + 1 - tids.sets_number;
    }
    return 0;
}

unsigned TIdUtil::Hash(const PartitionTIdList& tids) {
    return boost::hash_range(tids.tids.begin(), tids.tids.end()) + (tids.sets_number - 1);
}

unsigned TIdUtil::Hash(const SimpleTIdList& tids) {
    return boost::hash_range(tids.begin(), tids.end());
}

int TIdUtil::Support(const SimpleTIdList& tids) {
    return tids.size();
}
}  // namespace algos::cfd