#include "core/algorithms/cfd/util/prefix_tree.h"

#include <boost/functional/hash.hpp>

// see algorithms/cfd/LICENSE

namespace {

std::size_t HashCollection(algos::cfd::Itemset const& xs) {
    return boost::hash_range(xs.begin(), xs.end());
}

}  // namespace

namespace algos::cfd {

void PrefixTree::Insert(Itemset const& key, Itemset const& value) {
    PrefixNode* insertion_point = &root_;
    for (auto item : key) {
        PrefixNode* next = &insertion_point->sub_trees[item];
        if (!next->parent) {
            next->key = item;
            next->depth = insertion_point->depth + 1;
            next->parent = insertion_point;
        }
        insertion_point = next;
    }

    jumps_[HashCollection(key)].push_back(insertion_point);
    insertion_point->value = value;
}

Itemset const* PrefixTree::Find(Itemset const& key) const {
    auto it = jumps_.find(HashCollection(key));
    if (it == jumps_.end()) {
        return nullptr;
    }

    for (auto* prefix_node : it->second) {
        if (key.size() != prefix_node->depth) {
            continue;
        }
        PrefixNode* source = prefix_node;
        auto rit = key.rbegin();
        while (rit != key.rend() && prefix_node && prefix_node->key == *rit) {
            prefix_node = prefix_node->parent;
            ++rit;
        }
        if (rit == key.rend() && prefix_node == &root_) {
            return source->value.empty() ? nullptr : &source->value;
        }
    }
    return nullptr;
}

}  // namespace algos::cfd
