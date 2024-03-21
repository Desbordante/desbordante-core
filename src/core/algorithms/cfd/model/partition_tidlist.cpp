#include "partition_tidlist.h"

namespace algos::cfd {

int const PartitionTIdList::kSep = -1;

bool PartitionTIdList::operator==(PartitionTIdList const& b) const {
    return sets_number == b.sets_number && tids == b.tids;
}

bool PartitionTIdList::operator!=(PartitionTIdList const& b) const {
    return sets_number != b.sets_number || tids != b.tids;
}

bool PartitionTIdList::operator<(PartitionTIdList const& b) const {
    return sets_number < b.sets_number || (sets_number == b.sets_number && tids < b.tids);
}

SimpleTIdList PartitionTIdList::Convert() const {
    auto res = tids;
    if (sets_number > 1) {
        std::sort(res.begin(), res.end());
        res.erase(res.begin(), res.begin() + sets_number - 1);
    }
    return res;
}

PartitionTIdList PartitionTIdList::Intersection(PartitionTIdList const& rhs) const {
    std::unordered_map<int, int> eq_indices;
    eq_indices.reserve(this->tids.size() + 1 - this->sets_number);
    std::vector<std::vector<int> > eq_classes(this->sets_number);

    int eix = 0;
    int count = 0;
    for (unsigned ix = 0; ix <= this->tids.size(); ix++) {
        count++;
        if (ix == this->tids.size() || this->tids[ix] == PartitionTIdList::kSep) {
            eq_classes[eix].reserve(count);
            count = 0;
            eix++;
        } else {
            eq_indices[this->tids[ix]] = eix + 1;
        }
    }
    PartitionTIdList res;
    res.sets_number = 0;
    for (unsigned ix = 0; ix <= rhs.tids.size(); ix++) {
        if (ix == rhs.tids.size() || rhs.tids[ix] == PartitionTIdList::kSep) {
            for (auto& eqcl : eq_classes) {
                if (!eqcl.empty()) {
                    res.tids.insert(res.tids.end(), eqcl.begin(), eqcl.end());
                    res.tids.push_back(PartitionTIdList::kSep);
                    res.sets_number++;
                    eqcl.clear();
                }
            }
        } else {
            int const jt = rhs.tids[ix];
            if (eq_indices[jt]) {
                eq_classes[eq_indices[jt] - 1].push_back(jt);
            }
        }
    }
    if (!res.tids.empty() && res.tids.back() == PartitionTIdList::kSep) {
        res.tids.pop_back();
    }
    return res;
}

int PartitionTIdList::PartitionError(PartitionTIdList const& xa) const {
    int e = 0;

    std::map<int, int> bigt;
    int count = 0;
    for (unsigned pi = 0; pi <= xa.tids.size(); pi++) {
        if (pi == xa.tids.size() || xa.tids[pi] == PartitionTIdList::kSep) {
            bigt[xa.tids[pi - 1]] = count;
            count = 0;
        } else {
            count++;
        }
    }

    count = 0;
    int m = 0;
    for (unsigned cix = 0; cix <= this->tids.size(); cix++) {
        if (cix == this->tids.size() || this->tids[cix] == PartitionTIdList::kSep) {
            e += count - m;
            m = 0;
            count = 0;
        } else {
            count++;
            int t = this->tids[cix];
            if (bigt.count(t) && bigt[t] > m) {
                m = bigt[t];
            }
        }
    }
    return e;
}
}  // namespace algos::cfd
