#include "core/algorithms/fd/dfd/lattice_observations/lattice_observations.h"

NodeCategory LatticeObservations::UpdateDependencyCategory(boost::dynamic_bitset<> const& node) {
    NodeCategory new_category;
    if (node.count() <= 1) {
        new_category = NodeCategory::kMinimalDependency;
        (*this)[node] = new_category;
        return new_category;
    }

    auto column_indices = node;  // copy indices
    bool has_unchecked_subset = false;

    for (size_t index = column_indices.find_first(); index < column_indices.size();
         index = column_indices.find_next(index)) {
        column_indices[index] = false;  // remove one column
        auto const subset_node_iter = this->find(column_indices);

        if (subset_node_iter == this->end()) {
            // if we found unchecked subset of this node
            has_unchecked_subset = true;
        } else {
            NodeCategory const& subset_vertical_category = subset_node_iter->second;
            if (subset_vertical_category == NodeCategory::kMinimalDependency ||
                subset_vertical_category == NodeCategory::kDependency ||
                subset_vertical_category == NodeCategory::kCandidateMinimalDependency) {
                new_category = NodeCategory::kDependency;
                (*this)[node] = new_category;
                return new_category;
            }
        }

        column_indices[index] = true;  // restore removed column
    }
    new_category = has_unchecked_subset ? NodeCategory::kCandidateMinimalDependency
                                        : NodeCategory::kMinimalDependency;
    (*this)[node] = new_category;
    return new_category;
}

NodeCategory LatticeObservations::UpdateNonDependencyCategory(boost::dynamic_bitset<> const& node,
                                                              unsigned int rhs_index) {
    auto column_indices = node;
    column_indices[rhs_index] = true;
    column_indices.flip();

    NodeCategory new_category;
    bool has_unchecked_superset = false;

    for (size_t index = column_indices.find_first(); index < column_indices.size();
         index = column_indices.find_next(index)) {
        auto const superset_node_iter = this->find(boost::dynamic_bitset<>(node).set(index));

        if (superset_node_iter == this->end()) {
            // if we found unchecked superset of this node
            has_unchecked_superset = true;
        } else {
            NodeCategory const& superset_vertical_category = superset_node_iter->second;
            if (superset_vertical_category == NodeCategory::kMaximalNonDependency ||
                superset_vertical_category == NodeCategory::kNonDependency ||
                superset_vertical_category == NodeCategory::kCandidateMaximalNonDependency) {
                new_category = NodeCategory::kNonDependency;
                (*this)[node] = new_category;
                return new_category;
            }
        }
    }
    new_category = has_unchecked_superset ? NodeCategory::kCandidateMaximalNonDependency
                                          : NodeCategory::kMaximalNonDependency;
    (*this)[node] = new_category;
    return new_category;
}

bool LatticeObservations::IsCandidate(boost::dynamic_bitset<> const& node) const {
    auto node_iter = this->find(node);
    if (node_iter == this->end()) {
        return false;
    } else {
        return node_iter->second == NodeCategory::kCandidateMaximalNonDependency ||
               node_iter->second == NodeCategory::kCandidateMinimalDependency;
    }
}

std::unordered_set<boost::dynamic_bitset<>> LatticeObservations::GetUncheckedSubsets(
        boost::dynamic_bitset<> const& node, ColumnOrder const& column_order) const {
    auto indices = node;
    std::unordered_set<boost::dynamic_bitset<>> unchecked_subsets;

    for (int column_index : column_order.GetOrderHighDistinctCount(node)) {
        indices[column_index] = false;
        boost::dynamic_bitset<> subset_node = indices;
        if (this->find(subset_node) == this->end()) {
            unchecked_subsets.insert(std::move(subset_node));
        }
        indices[column_index] = true;
    }

    return unchecked_subsets;
}

std::unordered_set<boost::dynamic_bitset<>> LatticeObservations::GetUncheckedSupersets(
        boost::dynamic_bitset<> const& node, unsigned int rhs_index,
        ColumnOrder const& column_order) const {
    auto flipped_indices = ~node;
    std::unordered_set<boost::dynamic_bitset<>> unchecked_supersets;

    flipped_indices[rhs_index] = false;

    for (int column_index : column_order.GetOrderHighDistinctCount(flipped_indices)) {
        auto indices = node;

        indices[column_index] = true;
        boost::dynamic_bitset<> subset_node = indices;
        if (this->find(subset_node) == this->end()) {
            unchecked_supersets.insert(std::move(subset_node));
        }
    }

    return unchecked_supersets;
}
