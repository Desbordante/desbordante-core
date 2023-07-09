#pragma once

#include <utility>

#include "fd/cfd/structures/cfd_types.h"
#include "fd/cfd/util/tidlist_util.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

template <typename T>
struct MinerNode {
    MinerNode() : item(-1), node_supp(-1) {}

    explicit MinerNode(const Item& item) : item(item), node_supp(0) {}

    MinerNode(const Item& item, const T& tids) : item(item), tids(tids), node_supp(0) {
        node_supp = Supp();
    }

    MinerNode(const Item& item, const T& tids, unsigned supp)
        : item(item), tids(tids), node_supp(supp), prefix() {}

    MinerNode(const Item& item, const T& tids, unsigned supp, Itemset prefix)
        : item(item), tids(tids), node_supp(supp), prefix(std::move(prefix)) {}

    bool operator<(const MinerNode<T>& rhs) const {
        return tids < rhs.tids;
    }

    bool operator==(const MinerNode<T>& rhs) const {
        return item == rhs.item && prefix == rhs.prefix && tids == rhs.tids;
    }

    int Supp() const {
        if (node_supp) return node_supp;
        return TIdUtil::Support(tids);
    }

    Item item;
    T tids;
    unsigned node_supp;
    Itemset prefix;
    Itemset candidates;
};
}  // namespace algos::cfd
