#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <ranges>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/near/types.h"

namespace kingfisher {

// Represents a node of the branch-and-bound search tree.
struct Node {
    std::map<OFeatureIndex, std::shared_ptr<Node>> children;

    bool HasChild(OFeatureIndex feat_index) const {
        return children.contains(feat_index);
    }

    Node& GetChild(OFeatureIndex feat_index) {
        return *children.at(feat_index);
    }

    Node const& GetChild(OFeatureIndex feat_index) const {
        return *children.at(feat_index);
    }

    virtual void AddChild(OFeatureIndex feat_index, std::shared_ptr<Node> child_ptr) {
        children.emplace(feat_index, child_ptr);
    }

    // A 1 in a position means that the feature of that ordered index (or its negation
    // in case of n_possible) can be a consequence in this node or its children and will be
    // evaluated.
    boost::dynamic_bitset<> p_possible;
    boost::dynamic_bitset<> n_possible;
    // A 1 in a position means that a NodeAdress with that ordered index added is valid and a child
    // with that adress wasn't previously deleted
    boost::dynamic_bitset<> b_possible;
    // Best possible target function values for each feature as the consequence
    std::vector<double> p_best;
    std::vector<double> n_best;

    void Intersect(Node const& other) {
        p_possible &= other.p_possible;
        n_possible &= other.n_possible;
        std::ranges::transform(p_best, other.p_best, p_best.begin(),
                               [](double a, double b) { return std::min(a, b); });
        std::ranges::transform(n_best, other.n_best, n_best.begin(),
                               [](double a, double b) { return std::min(a, b); });
    }

    bool Pruned() const {
        return p_possible.none() && n_possible.none();
    }

    void Clear() {
        p_possible.clear();
        n_possible.clear();
        b_possible.clear();
        p_best.clear();
        n_best.clear();
    }

    Node(size_t feat_count, OFeatureIndex adds_feat)
        : p_possible(feat_count),
          n_possible(feat_count),
          b_possible(feat_count),
          p_best(feat_count, std::numeric_limits<double>::infinity()),
          n_best(feat_count, std::numeric_limits<double>::infinity()) {
        if (adds_feat + 1 < b_possible.size())
            // sets b_possible_ to 00...011...1 where the rightmost 0 has index adds_feat
            b_possible.set(adds_feat + 1, b_possible.size() - adds_feat - 1, true);
        p_possible.flip();
        n_possible.flip();
    }

    Node() = default;
};

}  // namespace kingfisher
