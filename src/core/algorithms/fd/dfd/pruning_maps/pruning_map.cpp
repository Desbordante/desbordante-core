#include "core/algorithms/fd/dfd/pruning_maps/pruning_map.h"

#include "core/model/index.h"

PruningMap::PruningMap(RelationalSchema const* schema) {
    for (model::Index column_index = 0; column_index != schema->GetNumColumns(); ++column_index) {
        try_emplace(std::move(boost::dynamic_bitset<>(schema->GetNumColumns()).set(column_index)));
    }
}

void PruningMap::Rebalance() {
    bool rebalanced_group = false;

    do {
        rebalanced_group = false;
        for (auto iter = this->begin(); iter != this->end();) {
            boost::dynamic_bitset<> const& key = iter->first;
            auto const& related_verticals = iter->second;

            // RebalanceGroup() invalidates this iterator, because it erases the key element
            ++iter;
            if (related_verticals.size() > 1000) {
                RebalanceGroup(key);
                rebalanced_group = true;
            }
        }
    } while (rebalanced_group);
}

void PruningMap::RebalanceGroup(boost::dynamic_bitset<> const& key) {
    auto const& deps_of_group = this->at(key);
    auto inverted_columns = ~key;

    for (size_t column_index = inverted_columns.find_first();
         column_index < inverted_columns.size();
         column_index = inverted_columns.find_next(column_index)) {
        boost::dynamic_bitset<> new_key = boost::dynamic_bitset<>(key).set(column_index);
        std::unordered_set<boost::dynamic_bitset<>> new_group;

        for (auto const& dep_of_group : deps_of_group) {
            if (new_key.is_subset_of(dep_of_group)) {
                new_group.insert(dep_of_group);
            }
        }

        this->insert(std::make_pair(std::move(new_key), std::move(new_group)));
    }
    this->erase(key);
}
