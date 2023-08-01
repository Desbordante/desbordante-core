#pragma once

#include "cfd_types.h"

namespace algos::cfd {

// Representation of П(I) from the paper.
// It is representation of partitioning the item set into equivalence classes
struct PartitionTIdList {
    PartitionTIdList() : sets_number(0) {}
    PartitionTIdList(SimpleTIdList tids, int nrSets) : tids(std::move(tids)), sets_number(nrSets) {}
    explicit PartitionTIdList(const SimpleTIdList& tidList) {
        tids = tidList;
        sets_number = 1;
    }

    SimpleTIdList tids;
    unsigned sets_number;
    static const int SEP;  // = -1;
    bool operator==(const PartitionTIdList&) const;
    bool operator!=(const PartitionTIdList&) const;
    bool operator<(const PartitionTIdList&) const;
    SimpleTIdList Convert() const;

    PartitionTIdList Intersection(const PartitionTIdList& rhs) const;

    int PartitionError(const PartitionTIdList&) const;
};
}  // namespace algos::cfd
