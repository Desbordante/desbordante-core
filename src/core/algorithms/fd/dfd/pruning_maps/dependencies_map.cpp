#include "core/algorithms/fd/dfd/pruning_maps/dependencies_map.h"

DependenciesMap::DependenciesMap(RelationalSchema const* schema) : PruningMap(schema) {}

std::unordered_set<boost::dynamic_bitset<>> DependenciesMap::GetPrunedSubsets(
        std::unordered_set<boost::dynamic_bitset<>> const& subsets) const {
    std::unordered_set<boost::dynamic_bitset<>> pruned_subsets;
    for (auto const& node : subsets) {
        if (CanBePruned(node)) {
            pruned_subsets.insert(node);
        }
    }
    return pruned_subsets;
}

void DependenciesMap::AddNewDependency(boost::dynamic_bitset<> const& node_to_add) {
    for (auto& map_row : *this) {
        boost::dynamic_bitset<> const& key = map_row.first;

        if (key.is_subset_of(node_to_add)) {
            auto& deps_for_key = map_row.second;
            bool has_subset_entry = false;

            for (auto iter = deps_for_key.begin(); iter != deps_for_key.end();) {
                // if verticals are the same, then contains == true
                boost::dynamic_bitset<> const& dep = *iter;
                if (dep.is_subset_of(node_to_add)) {
                    has_subset_entry = true;
                    break;
                } else if (node_to_add.is_subset_of(dep)) {
                    iter = deps_for_key.erase(iter);
                } else {
                    iter++;
                }
            }

            if (!has_subset_entry) {
                deps_for_key.insert(node_to_add);
            }
        }
    }
    Rebalance();
}

bool DependenciesMap::CanBePruned(boost::dynamic_bitset<> const& node) const {
    for (auto const& map_row : *this) {
        boost::dynamic_bitset<> const& key = map_row.first;
        if (key.is_subset_of(node)) {
            for (boost::dynamic_bitset<> const& dependency : map_row.second) {
                if (dependency.is_subset_of(node)) {
                    return true;
                }
            }
        }
    }
    return false;
}
