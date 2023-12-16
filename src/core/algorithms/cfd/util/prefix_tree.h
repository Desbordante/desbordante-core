#pragma once

#include <algorithm>
#include <iostream>
#include <map>
#include <queue>
#include <unordered_set>

#include <boost/functional/hash.hpp>

// see algorithms/cfd/LICENSE

namespace algos::cfd {

template <typename T>
size_t HashCollection(T const& xs) {
    size_t res = boost::hash_range(xs.begin(), xs.end());
    return res;
}

template <typename Key, typename Value>
class PrefixTree {
public:
    PrefixTree();
    void Insert(Key const&, Value const&);
    Value* Find(Key const&) const;

private:
    struct PrefixNode {
        Value value;
        std::map<typename Key::value_type, PrefixNode> sub_trees;
        PrefixNode* parent;
        typename Key::value_type key;
        int depth;
    };

    PrefixNode root_;
    std::unordered_set<size_t> hashes_;
    std::unordered_map<size_t, std::vector<PrefixNode*>> jumps_;
};

template <typename Key, typename Value>
PrefixTree<Key, Value>::PrefixTree() {
    root_.depth = 0;
    root_.value = Value();
}

template <typename Key, typename Value>
void PrefixTree<Key, Value>::Insert(Key const& k, Value const& v) {
    typename Key::const_iterator it = k.begin();
    PrefixNode* insertion_point = &root_;
    while (it != k.end()) {
        PrefixNode* next = &insertion_point->sub_trees[*it];
        if (!next->parent) {
            next->key = *it;
            next->depth = insertion_point->depth + 1;
            next->value = Value();
            next->parent = insertion_point;
        }
        insertion_point = next;
        it++;
    }
    jumps_[HashCollection(k)].push_back(insertion_point);
    insertion_point->value = v;
}

template <typename Key, typename Value>
Value* PrefixTree<Key, Value>::Find(Key const& k) const {
    auto const& elem = jumps_.find(HashCollection(k));
    if (elem == jumps_.end()) return nullptr;

    for (PrefixNode* prefix_node : elem->second) {
        if ((int)k.size() != prefix_node->depth) continue;
        PrefixNode* src = prefix_node;
        typename Key::const_reverse_iterator rit = k.rbegin();
        while (rit != k.rend() && prefix_node && prefix_node->key == *rit) {
            prefix_node = prefix_node->parent;
            rit++;
        }
        if (rit == k.rend() && prefix_node == &root_) {
            if (src->value != Value()) {
                return &src->value;
            } else {
                return nullptr;
            }
        }
    }
    return nullptr;
}
}  // namespace algos::cfd
