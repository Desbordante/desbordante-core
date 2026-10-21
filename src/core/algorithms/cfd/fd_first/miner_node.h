#pragma once

#include <utility>

#include "core/algorithms/cfd/model/cfd_types.h"

// see algorithms/cfd/fd_first/LICENSE

namespace algos::cfd {

template <typename T>
struct MinerNode {
    MinerNode() : item(-1), node_supp(-1) {}

    explicit MinerNode(Item const& item) : item(item), node_supp(0) {}

    MinerNode(Item const& item, T const& tids, unsigned supp)
        : item(item), tids(tids), node_supp(supp), prefix() {}

    MinerNode(Item const& item, T const& tids, unsigned supp, Itemset prefix)
        : item(item), tids(tids), node_supp(supp), prefix(std::move(prefix)) {}

    bool operator<(MinerNode<T> const& rhs) const {
        return tids < rhs.tids;
    }

    bool operator==(MinerNode<T> const& rhs) const {
        return item == rhs.item && prefix == rhs.prefix && tids == rhs.tids;
    }

    Item item;
    T tids;
    unsigned node_supp;
    Itemset prefix;
    Itemset candidates;
};
}  // namespace algos::cfd
