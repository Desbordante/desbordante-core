#include "core/algorithms/dd/fastdd/trees/minimize_tree.h"

namespace algos::dd {

std::optional<boost::dynamic_bitset<>> MinimizeTree::FindSuperset(
        boost::dynamic_bitset<> const& candidate, std::vector<std::size_t> const& nodes,
        std::size_t index) {
    if (!bitsets_.empty()) {
        for (auto const& bitset : bitsets_) {
            if (candidate.is_subset_of(bitset)) {
                return bitset;
            }
        }
    }

    if (index >= nodes.size()) {
        return std::nullopt;
    }

    std::size_t const next_node = nodes[index];
    auto const& child = children_[next_node];
    if (child) {
        std::optional<boost::dynamic_bitset<>> child_superset =
                child->FindSuperset(candidate, nodes, index + 1);
        if (child_superset) {
            return child_superset;
        }
    }

    return FindSuperset(candidate, nodes, index + 1);
}

void MinimizeTree::AddImpl(boost::dynamic_bitset<> const& candidate,
                           std::vector<std::size_t> const& nodes, std::size_t index) {
    if (index == nodes.size()) {
        bitsets_.push_back(candidate);
        return;
    }

    std::size_t const cur_node_id = nodes[index];
    auto& child = children_[cur_node_id];
    if (!child) {
        child = std::make_unique<MinimizeTree>();
    }
    child->AddImpl(candidate, nodes, index + 1);
}

std::optional<boost::dynamic_bitset<>> MinimizeTree::Add(boost::dynamic_bitset<> const& candidate,
                                                         std::vector<std::size_t> const& nodes) {
    std::optional<boost::dynamic_bitset<>> superset = FindSuperset(candidate, nodes, 0UL);

    if (superset) {
        return superset;
    }

    AddImpl(candidate, nodes, 0UL);
    return std::nullopt;
}

}  // namespace algos::dd
