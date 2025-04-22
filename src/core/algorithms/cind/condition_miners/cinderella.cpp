#include "cinderella.h"

#include <algorithm>
#include <map>

#include "cind/condition.h"
#include "cind/condition_miners/basket.h"
#include "cind/condition_type.h"

namespace algos::cind {
Cinderella::Cinderella(config::InputTables& input_tables) : CindMiner(input_tables) {}

CIND Cinderella::ExecuteSingle(model::IND const& aind) {
    auto attributes = ClassifyAttributes(aind);
    CIND cind{.ind = aind,
              .conditions = GetConditions(GetBaskets(attributes), attributes.conditional),
              .conditional_attributes = GetConditionalAttributesNames(attributes.conditional)};
    return cind;
}

std::vector<Basket> Cinderella::GetBaskets(Attributes const& attributes) {
    // algorithm uses modified left-outer join representation to build the baskets
    std::set<std::vector<int>> rhs_values;

    for (size_t index = 0; index < attributes.rhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<int> row;
        for (auto& attr : attributes.rhs_inclusion) {
            row.push_back(attr->GetValue(index));
        }
        rhs_values.insert(std::move(row));
    }

    std::vector<Basket> result;
    std::map<std::vector<int>, int> basket_id_by_value;
    for (size_t index = 0; index < attributes.lhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<int> row;
        for (auto& attr : attributes.lhs_inclusion) {
            row.push_back(attr->GetValue(index));
        }
        if (condition_type_._value == CondType::group) {
            if (auto const& it = basket_id_by_value.find(row); it == basket_id_by_value.cend()) {
                result.emplace_back(rhs_values.contains(row), std::unordered_set<Item>{});
                basket_id_by_value[row] = result.size() - 1;
            }
            auto basket_id = basket_id_by_value[row];
            for (auto const& cond_attr : attributes.conditional) {
                result[basket_id].items.emplace(cond_attr->GetColumnId(),
                                                cond_attr->GetValue(index));
            }
        } else {
            std::unordered_set<Item> basket_items;
            for (auto const& cond_attr : attributes.conditional) {
                basket_items.emplace(cond_attr->GetColumnId(), cond_attr->GetValue(index));
            }
            result.emplace_back(rhs_values.contains(row), std::move(basket_items));
        }
    }
    return result;
}

std::vector<Condition> Cinderella::GetConditions(std::vector<Basket> const& baskets,
                                                 AttrsType const& condition_attrs) const {
    std::set<Itemset> curr_itemsets;
    // scan all included baskets to extract 2-item itemsets
    // first item is Included indicator
    // second - is a condition attribute value from included basket

    // number of all included baskets - needed for computing completeness of conditions
    int included_baskets_cnt = 0;
    for (auto const& basket : baskets) {
        if (!basket.is_included) continue;
        ++included_baskets_cnt;
        for (auto const& item : basket.items) {
            curr_itemsets.emplace(std::vector<Item>{item}, true);
        }
    }
    // result stores all frequent and valid itemsets
    std::vector<Condition> result;
    // we need to keep only frequent 2-itemsets
    curr_itemsets = CreateNewItemsets(curr_itemsets, baskets, included_baskets_cnt, condition_attrs,
                                      result);
    while (!curr_itemsets.empty()) {
        // create k-sized candidates and keep only frequent of them
        curr_itemsets = CreateNewItemsets(GetCandidates(curr_itemsets), baskets,
                                          included_baskets_cnt, condition_attrs, result);
    }

    if (condition_type_._value == CondType::group) {
        std::vector<Condition> filtered_result;
        for (const auto& condition : result) {
            for (size_t row_id = 0; row_id < condition_attrs.back()->GetNumRows(); ++row_id) {
                bool is_matches = true;
                for (size_t attr_id = 0; attr_id < condition_attrs.size(); ++attr_id) {
                    if (condition.condition_attrs_values[attr_id] != kAnyValue && condition.condition_attrs_values[attr_id] != condition_attrs[attr_id]->GetStringValue(row_id)) {
                        is_matches = false;
                        break;
                    }
                }
                if (is_matches) {
                    filtered_result.emplace_back(condition);
                    break;
                }
            }
        }
        return filtered_result;
    }
    return result;
}

std::set<Itemset> Cinderella::CreateNewItemsets(std::set<Itemset> candidates,
                                                std::vector<Basket> const& baskets,
                                                int included_baskets_cnt,
                                                AttrsType const& condition_attrs,
                                                std::vector<Condition>& result) const {
    std::set<Itemset> new_itemsets;
    for (auto& candidate : candidates) {
        // number of included baskets our candidate contains in
        int included_contained_buskets_cnt = 0;
        // number of all baskets our candidate contains in
        int contained_buskets_cnt = 0;
        // count number of baskets candidate presented in
        for (auto const& basket : baskets) {
            if (basket.IsContains(candidate)) {
                ++contained_buskets_cnt;
                if (basket.is_included) {
                    ++included_contained_buskets_cnt;
                }
            }
        }
        double validity;
        if (contained_buskets_cnt) {
            validity = (double)included_contained_buskets_cnt / contained_buskets_cnt;
        } else {
            // negative value guarantees, that itemset will not be presented in answer
            validity = -1;
        }
        double completeness = (double)included_contained_buskets_cnt / included_baskets_cnt;
        // check completeness of candidate and add if it's frequent
        if (completeness >= min_completeness_) {
            // if candidate is valid - insert it into result itemsets.
            if (validity >= min_validity_) {
                result.emplace_back(candidate, condition_attrs, validity, completeness);
            }
            new_itemsets.insert(std::move(candidate));
        }
    }
    return new_itemsets;
}

std::set<Itemset> Cinderella::GetCandidates(std::set<Itemset> const& curr_itemsets) const {
    std::set<Itemset> candidates;
    for (auto const& lhs : curr_itemsets) {
        for (auto const& rhs : curr_itemsets) {
            // Intersect method returns empty Itemset, if that operation is not valid for those two
            // itemsets
            if (auto candidate = lhs.Intersect(rhs); candidate.GetSize() != 0) {
                std::set<Itemset> candidate_subsets = candidate.GetSubsets();
                // if any (k-1)-subset of candidate is not presented in (k-1)-itemsets from answer -
                // than our k-candidate is invalid. that's simple
                if (std::includes(curr_itemsets.begin(), curr_itemsets.end(),
                                  candidate_subsets.begin(), candidate_subsets.end())) {
                    candidates.insert(std::move(candidate));
                }
            }
        }
    }
    return candidates;
}
}  // namespace algos::cind
