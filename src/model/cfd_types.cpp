#include <algorithm>

#include "cfd_types.h"

const int PartitionTidList::SEP = -1;

bool PartitionTidList::operator==(const PartitionTidList& b) const {
    return sets_number == b.sets_number && tids == b.tids;
}

bool PartitionTidList::operator!=(const PartitionTidList& b) const {
    return sets_number != b.sets_number || tids != b.tids;
}

bool PartitionTidList::operator<(const PartitionTidList& b) const {
    return lessthan(*this, b);
}

Itemset itemset(int i) {
    Itemset res(1);
    res[0] = i;
    return res;
}

PartitionTidList convert(const SimpleTidList& tids) {
    return { tids, 1 };
}

SimpleTidList convert(const PartitionTidList& tids) {
    auto res = tids.tids;
    if (tids.sets_number > 1) {
        std::sort(res.begin(), res.end());
        res.erase(res.begin(), res.begin()+tids.sets_number -1);
    }
    return res;
}

bool lessthan(const PartitionTidList& lhs, const PartitionTidList& rhs) {
    return lhs.sets_number < rhs.sets_number || (lhs.sets_number == rhs.sets_number && lhs.tids < rhs.tids);
}

bool lessthan(const SimpleTidList& lhs, const SimpleTidList& rhs) {
    return lhs.size() > rhs.size();
}

bool equals(const PartitionTidList& lhs, const PartitionTidList& rhs) {
    return lhs.sets_number == rhs.sets_number && lhs.tids == rhs.tids;
}