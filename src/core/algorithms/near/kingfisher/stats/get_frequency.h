#pragma once

#include <algorithm>
#include <iostream>

#include "algorithms/near/kingfisher/node_adress.h"
#include "algorithms/near/types.h"
#include "model/transaction/transactional_data.h"

// Optimization possible: use PLI and cache frequencies
namespace kingfisher {

std::vector<FeatureIndex> GetFeatureFrequencyOrder(
        double min_occurences, model::TransactionalData const* transactional_data) {
    std::vector<std::pair<size_t, FeatureIndex>> feature_occurences;
    feature_occurences.reserve(transactional_data->GetUniverseSize());
    // iota
    for (size_t i = 0; i < transactional_data->GetUniverseSize(); ++i) {
        feature_occurences.emplace_back(0, i);
    }
    for (auto const& [_, itemset] : transactional_data->GetTransactions()) {
        for (auto const& item : itemset.GetItemsIDs()) {
            ++feature_occurences[item].first;
        }
    }

    std::sort(feature_occurences.begin(), feature_occurences.end(),
              [](auto const& a, auto const& b) { return a.first < b.first; });

    std::vector<FeatureIndex> feature_frequency_order;
    feature_frequency_order.reserve(feature_occurences.size());
    for (auto const& [occurences, index] : feature_occurences) {
        if (occurences < min_occurences) {
            break;
        }
        feature_frequency_order.push_back(index);
    }
    return feature_frequency_order;
}

// TODO: slow?
double GetItemsetOccurences(std::vector<FeatureIndex> const& itemset,
                            model::TransactionalData const* transactional_data) {
    size_t occurences = 0;
    for (auto const& [_, transaction] : transactional_data->GetTransactions()) {
        bool found = true;
        for (auto const& item : itemset) {
            auto const& item_ids = transaction.GetItemsIDs();
            if (std::find(item_ids.begin(), item_ids.end(), item) == item_ids.end()) {
                found = false;
                break;
            }
        }
        if (found) {
            ++occurences;
        }
    }
    return occurences;
}

// TODO: slow?
double GetItemsetFrequency(std::vector<FeatureIndex> const& itemset,
                           model::TransactionalData const* transactional_data) {
    return static_cast<double>(GetItemsetOccurences(itemset, transactional_data)) /
           transactional_data->GetTransactions().size();
}

// TODO: slow?
double GetNegativeItemsetOccurences(std::vector<FeatureIndex> const& itemset,
                                    model::TransactionalData const* transactional_data) {
    size_t occurences = 0;
    for (auto const& [_, transaction] : transactional_data->GetTransactions()) {
        bool found_none = true;
        for (auto const& item : itemset) {
            auto const& item_ids = transaction.GetItemsIDs();
            if (std::find(item_ids.begin(), item_ids.end(), item) != item_ids.end()) {
                found_none = false;
                break;
            }
        }
        if (found_none) {
            ++occurences;
        }
    }
    return occurences;
}

// TODO: slow?
double GetNegativeItemsetFrequency(std::vector<FeatureIndex> const& itemset,
                                   model::TransactionalData const* transactional_data) {
    return static_cast<double>(GetNegativeItemsetOccurences(itemset, transactional_data)) /
           transactional_data->GetTransactions().size();
}

// TODO: slow?
double GetConsMatches(Consequence cons, model::TransactionalData const* transactional_data) {
    if (cons.positive) {
        return GetItemsetOccurences({cons.feature}, transactional_data);
    } else {
        return GetNegativeItemsetOccurences({cons.feature}, transactional_data);
    }
}

// Count rule occurrences: antecedent items must be present; if consequent positive, it must be
// present, otherwise absent.
double GetRuleOccurences(model::NeARIDs const& near,
                         model::TransactionalData const* transactional_data) {
    size_t occurrences = 0;
    for (auto const& [_, transaction] : transactional_data->GetTransactions()) {
        auto const& item_ids = transaction.GetItemsIDs();
        // Check antecedent items presence
        bool ante_present = true;
        for (auto const& item : near.ante) {
            if (std::find(item_ids.begin(), item_ids.end(), item) == item_ids.end()) {
                ante_present = false;
                break;
            }
        }
        if (!ante_present) {
            continue;
        }
        // Check consequent
        bool cons_in_tx =
                (std::find(item_ids.begin(), item_ids.end(), near.cons.feature) != item_ids.end());
        if ((near.cons.positive && cons_in_tx) || (!near.cons.positive && !cons_in_tx)) {
            ++occurrences;
        }
    }
    return occurrences;
}

double GetRuleFrequency(model::NeARIDs const& near,
                        model::TransactionalData const* transactional_data) {
    return static_cast<double>(GetRuleOccurences(near, transactional_data)) /
           transactional_data->GetTransactions().size();
}

// Count occurrences where antecedent items are absent; consequent logic as in GetRuleOccurences.
double GetNegRuleOccurences(model::NeARIDs const& near,
                            model::TransactionalData const* transactional_data) {
    size_t occurrences = 0;
    for (auto const& [_, transaction] : transactional_data->GetTransactions()) {
        auto const& item_ids = transaction.GetItemsIDs();
        // Check antecedent items absence
        bool ante_absent = true;
        for (auto const& item : near.ante) {
            if (std::find(item_ids.begin(), item_ids.end(), item) != item_ids.end()) {
                ante_absent = false;
                break;
            }
        }
        if (!ante_absent) {
            continue;
        }
        // Check consequent
        bool cons_in_tx =
                (std::find(item_ids.begin(), item_ids.end(), near.cons.feature) != item_ids.end());
        if ((near.cons.positive && cons_in_tx) || (!near.cons.positive && !cons_in_tx)) {
            ++occurrences;
        }
    }
    return occurrences;
}

// Gets the minimum frequency a feature must have to be able to appear in significant rules
size_t GetMinOccurences(double max_p, model::TransactionalData const* transactional_data) {
    size_t row_count = transactional_data->GetNumTransactions();
    for (size_t i = 1; i < transactional_data->GetNumTransactions() / 2 + 1; i++) {
        double lb1gamma = std::lgamma(i) + std::lgamma(row_count - i) - std::lgamma(row_count) -
                          std::lgamma(max_p);
        if (lb1gamma > 0.0) {
            return i;
        }
    }
    return 0;
}

}  // namespace kingfisher
