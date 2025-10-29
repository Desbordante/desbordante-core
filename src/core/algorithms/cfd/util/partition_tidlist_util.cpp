#include "partition_tidlist_util.h"

#include <unordered_map>

#include "cfd/model/partition_tidlist.h"
#include "tidlist_util.h"

namespace algos::cfd {

// Computes intersection
std::vector<PartitionTIdList> PartitionTIdListUtil::ConstructIntersection(
        PartitionTIdList const& lhs, std::vector<PartitionTIdList const*> const& rhses) {
    std::unordered_map<int, int> eq_indices(TIdUtil::Support(lhs.tids));
    std::vector<std::vector<int> > eq_classes(lhs.sets_number);
    // Construct a lookup from tid to equivalence class
    int eix = 0;
    int count = 0;
    for (unsigned ix = 0; ix <= lhs.tids.size(); ix++) {
        count++;
        if (ix == lhs.tids.size() || lhs.tids[ix] == PartitionTIdList::kSep) {
            eq_classes[eix++].reserve(count);
            count = 0;
        } else {
            eq_indices[lhs.tids[ix]] = eix + 1;
        }
    }
    std::vector<PartitionTIdList> res;
    for (PartitionTIdList const* rhs : rhses) {
        PartitionTIdList p_tid_list = PartitionTIdList();
        p_tid_list.sets_number = 0;
        p_tid_list.tids.reserve(lhs.tids.size());
        for (unsigned ix = 0; ix <= rhs->tids.size(); ix++) {
            if (ix == rhs->tids.size() || rhs->tids[ix] == PartitionTIdList::kSep) {
                for (auto& eqcl : eq_classes) {
                    if (!eqcl.empty()) {
                        p_tid_list.tids.insert(p_tid_list.tids.end(), eqcl.begin(), eqcl.end());
                        p_tid_list.tids.push_back(PartitionTIdList::kSep);
                        p_tid_list.sets_number++;
                        eqcl.clear();
                    }
                }
            } else {
                int jt = rhs->tids[ix];
                if (eq_indices.count(jt)) {
                    eq_classes[eq_indices[jt] - 1].push_back(jt);
                }
            }
        }

        if (!p_tid_list.tids.empty() && p_tid_list.tids.back() == PartitionTIdList::kSep) {
            p_tid_list.tids.pop_back();
        }
        res.push_back(p_tid_list);
    }
    return res;
}

}  // namespace algos::cfd
