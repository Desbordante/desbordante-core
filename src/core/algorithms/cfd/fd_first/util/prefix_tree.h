#pragma once

#include <cstddef>
#include <map>
#include <unordered_map>
#include <vector>

#include "core/algorithms/cfd/model/cfd_types.h"

// see algorithms/cfd/fd_first/LICENSE

namespace algos::cfd {

class PrefixTree {
private:
    struct PrefixNode {
        Itemset value;
        std::map<Item, PrefixNode> sub_trees;
        PrefixNode* parent = nullptr;
        Item key{};
        size_t depth = 0;
    };

    PrefixNode root_;
    std::unordered_map<size_t, std::vector<PrefixNode*>> jumps_;

public:
    PrefixTree() = default;
    void Insert(Itemset const&, Itemset const&);
    Itemset const* Find(Itemset const&) const;
};
}  // namespace algos::cfd
