#include "partition_util.h"

#include <numeric>

// see algorithms/cfd/LICENSE

namespace algos::cfd {

unsigned PartitionUtil::GetPartitionSupport(const SimpleTIdList& pids,
                                            const std::vector<unsigned>& psupps) {
    unsigned res = 0;
    for (int p : pids) {
        res += psupps[p];
    }
    return res;
}

unsigned PartitionUtil::GetPartitionError(const SimpleTIdList& pids,
                                          const PartitionList& partitions) {
    unsigned res = 0;
    for (int p : pids) {
        unsigned max =
                *(std::max_element(partitions[p].second.begin(), partitions[p].second.end()));
        unsigned total =
                std::accumulate(partitions[p].second.begin(), partitions[p].second.end(), 0u);
        res += total - max;
    }
    return res;
}

bool PartitionUtil::IsConstRulePartition(const SimpleTIdList& items, const RhsesPair2DList& rhses) {
    int rhs_value;
    bool first = true;
    for (int pi : items) {
        if (first) {
            first = false;
            rhs_value = rhses[pi][0].first;
        }
        for (auto rp : rhses[pi]) {
            int r = rp.first;
            if (r != rhs_value) {
                return false;
            }
        }
    }
    return true;
}
}  // namespace algos::cfd
