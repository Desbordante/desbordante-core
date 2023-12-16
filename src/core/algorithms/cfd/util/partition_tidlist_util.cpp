#include "partition_tidlist_util.h"

#include "tidlist_util.h"

namespace algos::cfd {

// Computes intersection
std::vector<PartitionTIdList> PartitionTIdListUtil::ConstructIntersection(
        PartitionTIdList const& lhs, std::vector<PartitionTIdList const*> const& rhses) {
    std::unordered_map<int, int> eqIndices(TIdUtil::Support(lhs.tids));
    std::vector<std::vector<int> > eqClasses(lhs.sets_number);
    // Construct a lookup from tid to equivalence class
    int eix = 0;
    int count = 0;
    for (unsigned ix = 0; ix <= lhs.tids.size(); ix++) {
        count++;
        if (ix == lhs.tids.size() || lhs.tids[ix] == PartitionTIdList::SEP) {
            eqClasses[eix++].reserve(count);
            count = 0;
        } else {
            eqIndices[lhs.tids[ix]] = eix + 1;
        }
    }
    std::vector<PartitionTIdList> res;
    for (PartitionTIdList const* rhs : rhses) {
        PartitionTIdList p_tid_list = PartitionTIdList();
        p_tid_list.sets_number = 0;
        p_tid_list.tids.reserve(lhs.tids.size());
        for (unsigned ix = 0; ix <= rhs->tids.size(); ix++) {
            if (ix == rhs->tids.size() || rhs->tids[ix] == PartitionTIdList::SEP) {
                for (auto& eqcl : eqClasses) {
                    if (!eqcl.empty()) {
                        p_tid_list.tids.insert(p_tid_list.tids.end(), eqcl.begin(), eqcl.end());
                        p_tid_list.tids.push_back(PartitionTIdList::SEP);
                        p_tid_list.sets_number++;
                        eqcl.clear();
                    }
                }
            } else {
                int jt = rhs->tids[ix];
                if (eqIndices.count(jt)) {
                    eqClasses[eqIndices[jt] - 1].push_back(jt);
                }
            }
        }

        if (!p_tid_list.tids.empty() && p_tid_list.tids.back() == PartitionTIdList::SEP) {
            p_tid_list.tids.pop_back();
        }
        res.push_back(p_tid_list);
    }
    return res;
}

}  // namespace algos::cfd
