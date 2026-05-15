#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/algorithms/dd/fastdd/util/bitset_concept.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class MinimizeTree {
private:
    std::vector<std::unique_ptr<MinimizeTree>> children_;
    std::vector<Bitset> bitsets_;
    Bitset children_bitset_;

    bool FindSuperset(Bitset const& candidate, std::vector<std::size_t> const& nodes,
                      std::size_t index) const {
        for (auto const& bitset : bitsets_) {
            if (candidate.is_subset_of(bitset)) {
                return true;
            }
        }

        for (std::size_t cur_index = index; cur_index != nodes.size(); ++cur_index) {
            std::size_t const next_node = nodes[cur_index];
            if (children_bitset_[next_node]) {
                if (children_[next_node]->FindSuperset(candidate, nodes, cur_index + 1)) {
                    return true;
                }
            }
        }

        return false;
    }

    void AddImpl(Bitset const& candidate, std::vector<std::size_t> const& nodes,
                 std::size_t index) {
        if (index == nodes.size()) {
            bitsets_.push_back(candidate);
            return;
        }

        std::size_t const cur_node_id = nodes[index];
        if (children_.empty()) {
            children_.resize(candidate.size());
        }
        auto& child = children_[cur_node_id];
        if (!child) {
            child = std::make_unique<MinimizeTree>(candidate.size());
            children_bitset_.set(cur_node_id, true);
        }
        child->AddImpl(candidate, nodes, index + 1);
    }

public:
    MinimizeTree(std::size_t size) : children_(), bitsets_(), children_bitset_(size) {}

    bool Add(Bitset const& candidate, std::vector<std::size_t> const& nodes) {
        bool superset_found = FindSuperset(candidate, nodes, 0UL);

        if (superset_found) {
            return true;
        }

        AddImpl(candidate, nodes, 0UL);
        return false;
    }
};

}  // namespace algos::dd
