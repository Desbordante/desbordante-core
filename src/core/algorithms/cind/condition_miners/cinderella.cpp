#include "cinderella.h"

#include <algorithm>
#include <map>

#include "cind/condition_miners/basket.h"
#include "cind/condition_type.h"

namespace algos::cind {
Cinderella::Cinderella(config::InputTables& input_tables) : CindMiner(input_tables) {}

CIND Cinderella::ExecuteSingle(model::IND const& aind) {
    auto attributes = ClassifyAttributes(aind);
    auto baskets = GetBaskets(attributes);

    for (auto const& basket : baskets) {
        fprintf(stderr, "basket: [%s, (", basket.is_included ? "included" : "null");
        for (auto const& item : basket.items) {
            fprintf(stderr, "{%s, %s}, ",
                    tables_.GetTable(aind.GetLhs().GetTableIndex())
                            .GetColumnData(item.column_id)
                            .GetColumn()
                            ->GetName()
                            .c_str(),
                    tables_.GetTable(aind.GetLhs().GetTableIndex())
                            .GetColumnData(item.column_id)
                            .DecodeValue(item.value)
                            .c_str());
        }
        fprintf(stderr, ")]\n");
    }
    fprintf(stderr, "\n");
    CIND cind{.ind = aind,
              .conditions = GetConditions(baskets, attributes.conditional),
              .conditional_attributes = GetConditionalAttributesNames(attributes.conditional)};
    fprintf(stderr, "Result:\n%s", cind.ToString().c_str());
    return cind;
}

std::vector<Basket> Cinderella::GetBaskets(Attributes const& attributes) {
    // algorithm uses modified left-outer join representation to build the baskets
    fprintf(stderr, "lhs inclusion attributes: [");
    for (auto const attr : attributes.lhs_inclusion) {
        fprintf(stderr, "(%u, %u)", attr->GetTableId(), attr->GetColumnId());
    }
    fprintf(stderr, "]\n");
    fprintf(stderr, "rhs inclusion attributes: [");
    for (auto const attr : attributes.rhs_inclusion) {
        fprintf(stderr, "(%u, %u)", attr->GetTableId(), attr->GetColumnId());
    }
    fprintf(stderr, "]\n");
    fprintf(stderr, "conditional attributes: [");
    for (auto const attr : attributes.conditional) {
        fprintf(stderr, "(%u, %u)", attr->GetTableId(), attr->GetColumnId());
    }
    fprintf(stderr, "]\n");
    std::set<std::vector<int>> rhs_values;

    fprintf(stderr, "rhs values: [");
    for (size_t index = 0; index < attributes.rhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<int> row;
        fprintf(stderr, "{");
        for (auto& attr : attributes.rhs_inclusion) {
            row.push_back(attr->GetValue(index));
            fprintf(stderr, "%s, ", attr->GetStringValue(index).c_str());
        }
        fprintf(stderr, "}");
        rhs_values.insert(std::move(row));
    }
    fprintf(stderr, "]\n");

    std::vector<Basket> result;
    std::map<std::vector<int>, Basket*> baskets_by_value;
    fprintf(stderr, "lhs values: [");
    for (size_t index = 0; index < attributes.lhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<int> row;
        fprintf(stderr, "{");
        for (auto& attr : attributes.lhs_inclusion) {
            row.push_back(attr->GetValue(index));
            fprintf(stderr, "%s, ", attr->GetStringValue(index).c_str());
        }
        fprintf(stderr, "}");
        if (condition_type_._value == CondType::group) {
            if (auto const& it = baskets_by_value.find(row); it == baskets_by_value.cend()) {
                result.push_back({.is_included = rhs_values.contains(row), .items = {}});
                baskets_by_value[row] = &result[result.size() - 1];
            }
            for (auto const& cond_attr : attributes.conditional) {
                baskets_by_value[row]->items.insert({.column_id = cond_attr->GetColumnId(),
                                                     .value = cond_attr->GetValue(index)});
            }
        } else {
            std::unordered_set<Item> basket_items;
            for (auto const& cond_attr : attributes.conditional) {
                basket_items.insert({.column_id = cond_attr->GetColumnId(),
                                     .value = cond_attr->GetValue(index)});
            }
            result.push_back(
                    {.is_included = rhs_values.contains(row), .items = std::move(basket_items)});
        }
        if (rhs_values.contains(row)) {
            fprintf(stderr, "::included");
        }
        fprintf(stderr, ", ");
    }
    fprintf(stderr, "]\n");
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
    return result;
}

std::set<Itemset> Cinderella::CreateNewItemsets(std::set<Itemset> candidates,
                                                std::vector<Basket> const& baskets,
                                                int included_baskets_cnt,
                                                AttrsType const& condition_attrs,
                                                std::vector<Condition>& result) const {
    fprintf(stderr, "candidates:\n");
    std::set<Itemset> new_itemsets;
    for (auto& candidate : candidates) {
        fprintf(stderr, "%s, ", candidate.ToString().c_str());
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
        fprintf(stderr, ", validity: %f, completeness: %f", validity, completeness);
        // check completeness of candidate and add if it's frequent
        if (completeness >= min_completeness_ - 1e-6) {
            // if candidate is valid - insert it into result itemsets.
            if (validity >= min_validity_ - 1e-6) {
                result.emplace_back(candidate, condition_attrs, validity, completeness);
                fprintf(stderr, ", is in result");
            } else {
                fprintf(stderr, ", is frequent itemset");
            }
            new_itemsets.insert(std::move(candidate));
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
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
