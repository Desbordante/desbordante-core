#pragma once

#include <numeric>

#include "core/algorithms/cfd/model/cfd_types.h"

namespace algos::cfd {

// Representation of П(I) from the paper.
// It is representation of partitioning the item set into equivalence classes
struct PartitionTIdList {
    PartitionTIdList() : sets_number(0) {}

    PartitionTIdList(SimpleTIdList tids, int nrSets) : tids(std::move(tids)), sets_number(nrSets) {}

    explicit PartitionTIdList(std::size_t num_rows) : tids(num_rows), sets_number(1) {
        std::iota(tids.begin(), tids.end(), 0);
    }

    explicit PartitionTIdList(SimpleTIdList const& tidList) {
        tids = tidList;
        sets_number = 1;
    }

    SimpleTIdList tids;
    unsigned sets_number;
    static constexpr int kSep = -1;
    bool operator==(PartitionTIdList const&) const;
    bool operator!=(PartitionTIdList const&) const;
    bool operator<(PartitionTIdList const&) const;
    SimpleTIdList Convert() const;

    PartitionTIdList Intersection(PartitionTIdList const& rhs) const;
    std::vector<PartitionTIdList> Intersection(
            std::vector<PartitionTIdList const*> const& rhses) const;

    int PartitionError(PartitionTIdList const&) const;
};
}  // namespace algos::cfd
