#include "cinderella.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <tuple>
#include <unordered_map>

#include "cind/condition.h"
#include "cind/condition_miners/basket.h"
#include "cind/condition_type.h"

namespace algos::cind {
namespace {
std::list<BasketInfo> MergeBaskets(std::list<BasketInfo> const& baskets1,
                                   std::list<BasketInfo> const& baskets2) {
    auto it1 = baskets1.begin();
    auto it2 = baskets2.begin();
    std::list<BasketInfo> result;
    while (it1 != baskets1.end() && it2 != baskets2.end()) {
        auto const& [index1, positions1, is_included1] = *it1;
        auto const& [index2, positions2, is_included2] = *it2;
        if (index1 < index2) {
            ++it1;
        } else if (index1 > index2) {
            ++it2;
        } else {
            std::list<size_t> positions_intersection;
            std::set_intersection(positions1.begin(), positions1.end(), positions2.begin(),
                                  positions2.end(), std::back_inserter(positions_intersection));
            if (!positions_intersection.empty()) {
                result.emplace_back(index1, positions_intersection, is_included1);
            }
            ++it1;
            ++it2;
        }
    }
    return result;
}
}  // namespace

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
                result.emplace_back(rhs_values.contains(row), ItemsInfo{});
                basket_id_by_value[row] = basket_id_by_value.size();
            }
            auto basket_id = basket_id_by_value[row];
            for (auto const& cond_attr : attributes.conditional) {
                Item item{cond_attr->GetColumnId(), cond_attr->GetValue(index)};
                result[basket_id].items[item].push_back(index);
            }
        } else {
            ItemsInfo basket_items;
            for (auto const& cond_attr : attributes.conditional) {
                Item item{cond_attr->GetColumnId(), cond_attr->GetValue(index)};
                basket_items[item].push_back(index);
            }
            result.emplace_back(rhs_values.contains(row), std::move(basket_items));
        }
    }
    return result;
}

std::vector<Condition> Cinderella::GetConditions(std::vector<Basket> const& baskets,
                                                 AttrsType const& condition_attrs) const {
    std::unordered_map<Item, std::list<BasketInfo>> first_level_items;
    // scan all included baskets to extract all included items
    // number of all included baskets - needed for computing completeness of conditions
    int included_baskets_cnt = 0;
    for (auto const& basket : baskets) {
        if (!basket.is_included) continue;
        ++included_baskets_cnt;
        for (auto const& [item, _] : basket.items) {
            first_level_items.try_emplace(item, std::list<BasketInfo>{});
        }
    }

    for (size_t basket_id = 0; basket_id < baskets.size(); ++basket_id) {
        auto const& basket = baskets.at(basket_id);
        for (auto const& [item, positions] : basket.items) {
            if (auto const& it = first_level_items.find(item); it != first_level_items.end()) {
                it->second.emplace_back(basket_id, positions, basket.is_included);
            }
        }
    }

    // result stores all frequent and valid itemsets
    std::vector<Condition> result;

    Itemset itemset(std::move(first_level_items), included_baskets_cnt, min_completeness_);
    while (!itemset.GetItems().empty()) {
        for (auto candidate : itemset.GetItems()) {
            if (candidate->GetValidity() >= min_validity_) {
                result.emplace_back(candidate, condition_attrs);
            }
        }
        CreateNewItemsets(itemset);
    }
    return result;
}

void Cinderella::CreateNewItemsets(Itemset& itemset) const {
    std::vector<std::tuple<std::shared_ptr<ItemsetNode>, Item, std::list<BasketInfo>>>
            new_items_info;
    for (auto const& itemset_prefix : itemset.GetPrevItems()) {
        std::vector<Item> candidate = itemset_prefix->GetContents();
        for (std::shared_ptr<ItemsetNode> parent : itemset_prefix->GetChildNodes()) {
            candidate.push_back(parent->GetValue());
            for (auto const& elem : itemset_prefix->GetChildNodes()) {
                if (parent->GetValue().column_id < elem->GetValue().column_id) {
                    candidate.push_back(elem->GetValue());
                    if (itemset.CheckSubsets(candidate)) {
                        new_items_info.emplace_back(
                                parent, elem->GetValue(),
                                MergeBaskets(parent->GetBaskets(), elem->GetBaskets()));
                    }
                    candidate.pop_back();
                }
            }
            candidate.pop_back();
        }
    }

    itemset.CreateNewLayer(new_items_info);
}
}  // namespace algos::cind
