#include "core/algorithms/fd/dfd/pruning_maps/non_dependencies_map.h"

NonDependenciesMap::NonDependenciesMap(RelationalSchema const* schema) : PruningMap(schema) {}

std::unordered_set<boost::dynamic_bitset<>> NonDependenciesMap::GetPrunedSupersets(
        std::unordered_set<boost::dynamic_bitset<>> const& supersets) const {
    std::unordered_set<boost::dynamic_bitset<>> pruned_supersets;
    for (auto const& node : supersets) {
        if (CanBePruned(node)) {
            pruned_supersets.insert(node);
        }
    }
    return pruned_supersets;
}

bool NonDependenciesMap::CanBePruned(boost::dynamic_bitset<> const& node) const {
    for (auto const& map_row : *this) {
        boost::dynamic_bitset<> const& key = map_row.first;
        if (key.is_subset_of(node)) {
            for (boost::dynamic_bitset<> const& non_dependency : map_row.second) {
                if (node.is_subset_of(non_dependency)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void NonDependenciesMap::AddNewNonDependency(boost::dynamic_bitset<> const& node_to_add) {
    for (auto& map_row : *this) {
        boost::dynamic_bitset<> const& key = map_row.first;

        if (key.is_subset_of(node_to_add)) {
            auto& non_deps_for_key = map_row.second;
            bool has_superset_entry = false;

            for (auto iter = non_deps_for_key.begin(); iter != non_deps_for_key.end();) {
                // if verticals are the same, then contains == true
                boost::dynamic_bitset<> const& non_dep = *iter;
                if (node_to_add.is_subset_of(non_dep)) {
                    has_superset_entry = true;
                    break;
                } else if (non_dep.is_subset_of(node_to_add)) {
                    iter = non_deps_for_key.erase(iter);
                } else {
                    iter++;
                }
            }

            if (!has_superset_entry) {
                non_deps_for_key.insert(node_to_add);
            }
        }
    }
    Rebalance();
}
