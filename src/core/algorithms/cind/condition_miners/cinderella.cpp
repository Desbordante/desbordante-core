#include "cinderella.h"

#include <map>
#include <tuple>

#include "cind/condition.h"
#include "cind/condition_miners/basket.h"
#include "cind/condition_type.h"

namespace algos::cind {
namespace {
std::list<BasketInfo> MergeBaskets(std::list<BasketInfo> const& baskets1,
                                   std::list<BasketInfo> const& baskets2) {
    // logg("MergeBaskets begin\n");
    auto it1 = baskets1.begin();
    auto it2 = baskets2.begin();
    std::list<BasketInfo> result;
    while (it1 != baskets1.end() && it2 != baskets2.end()) {
        size_t index_1 = std::get<0>(*it1);
        size_t index_2 = std::get<0>(*it2);
        if (index_1 < index_2) {
            ++it1;
        } else if (index_1 > index_2) {
            ++it2;
        } else {
            result.push_back(*it1);
            ++it1;
            ++it2;
        }
    }
    // logg("MergeBaskets end\n");
    return result;
}
}  // namespace

Cinderella::Cinderella(config::InputTables& input_tables) : CindMiner(input_tables) {}

CIND Cinderella::ExecuteSingle(model::IND const& aind) {
    auto attributes = ClassifyAttributes(aind);
    CIND cind{.ind = aind,
              .conditions = GetConditions(GetBaskets(attributes), attributes.conditional),
              .conditional_attributes = GetConditionalAttributesNames(attributes.conditional)};
    fprintf(stderr, "cinderella ExecuteSingle %s complete\n", aind.ToShortString().c_str());
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
        size_t real_id = index;
        if (condition_type_._value == CondType::group) {
            if (auto const& it = basket_id_by_value.find(row); it == basket_id_by_value.cend()) {
                real_id = basket_id_by_value.size();
                basket_id_by_value[row] = real_id;
                // logg("!! real_id: %zu\n", real_id);
            } else {
                real_id = it->second;
                // logg("~~ real_id: %zu\n", real_id);
            }
        }
        std::unordered_set<Item> basket_items;
        for (auto const& cond_attr : attributes.conditional) {
            basket_items.emplace(cond_attr->GetColumnId(), cond_attr->GetValue(index));
        }
        result.emplace_back(rhs_values.contains(row), real_id, std::move(basket_items));
    }
    return result;
}

std::vector<Condition> Cinderella::GetConditions(std::vector<Basket> const& baskets,
                                                 AttrsType const& condition_attrs) const {
    // logg("GetConditions begin\n");
    std::unordered_map<Item, std::list<BasketInfo>> first_level_items;
    // scan all included baskets to extract all included items
    // number of all included baskets - needed for computing completeness of conditions
    std::unordered_set<size_t> included_baskets_ids;
    for (auto const& basket : baskets) {
        if (!basket.is_included) continue;
        included_baskets_ids.insert(basket.real_id);
        for (auto const& item : basket.items) {
            first_level_items.try_emplace(item, std::list<BasketInfo>{});
        }
    }
    // logg("GetConditions 1\n");

    for (size_t basket_id = 0; basket_id < baskets.size(); ++basket_id) {
        // logg("%zu : %zu\n", basket_id, baskets[basket_id].real_id);
        auto const& basket = baskets.at(basket_id);
        for (auto const& item : basket.items) {
            if (const auto &it = first_level_items.find(item); it != first_level_items.end()) {
                it->second.emplace_back(basket_id, basket.real_id, basket.is_included);
            }
        }
    }
    // logg("GetConditions 2\n");

    // result stores all frequent and valid itemsets
    std::vector<Condition> result;

    Itemset itemset(std::move(first_level_items), included_baskets_ids.size(), min_completeness_);
    // logg("GetConditions 3\n");
    while (!itemset.GetItems().empty()) {
        // logg("GetConditions 4.1\n");
        for (auto candidate : itemset.GetItems()) {
            if (candidate->GetValidity() >= min_validity_) {
                result.emplace_back(candidate, condition_attrs);
            }
        }
        // logg("GetConditions 4.2\n");

        CreateNewItemsets(itemset);
        // logg("GetConditions 4.3\n");
    }
    // logg("GetConditions 5\n");

    // if (condition_type_._value == CondType::group) {
    //     std::vector<Condition> filtered_result;
    //     for (auto const& condition : result) {
    //         for (size_t row_id = 0; row_id < condition_attrs.back()->GetNumRows(); ++row_id) {
    //             bool is_matches = true;
    //             for (size_t attr_id = 0; attr_id < condition_attrs.size(); ++attr_id) {
    //                 if (condition.condition_attrs_values[attr_id] != kAnyValue &&
    //                     condition.condition_attrs_values[attr_id] !=
    //                             condition_attrs[attr_id]->GetStringValue(row_id)) {
    //                     is_matches = false;
    //                     break;
    //                 }
    //             }
    //             if (is_matches) {
    //                 filtered_result.emplace_back(condition);
    //                 break;
    //             }
    //         }
    //     }
    //     return filtered_result;
    // }
    // logg("GetConditions end\n");
    return result;
}

void Cinderella::CreateNewItemsets(Itemset& itemset) const {
    std::vector<std::tuple<std::shared_ptr<ItemsetNode>, Item, std::list<BasketInfo>>>
            new_items_info;
    // logg("CreateNewItemsets begin\n");
    for (auto const& itemset_prefix : itemset.GetPrevItems()) {
        std::vector<Item> candidate = itemset_prefix->GetContents();
        // logg("CreateNewItemsets 1.1\n");
        for (std::shared_ptr<ItemsetNode> parent : itemset_prefix->GetChildNodes()) {
            candidate.push_back(parent->GetValue());
            // logg("CreateNewItemsets 1.1.1\n");
            for (auto const& elem : itemset_prefix->GetChildNodes()) {
                if (parent->GetValue().column_id < elem->GetValue().column_id) {
                    // logg("CreateNewItemsets 1.1.1.1\n");
                    candidate.push_back(elem->GetValue());
                    if (itemset.CheckSubsets(candidate)) {
                        // logg("CreateNewItemsets 1.1.1.1.1\n");
                        new_items_info.emplace_back(
                                parent, elem->GetValue(),
                                MergeBaskets(parent->GetBaskets(), elem->GetBaskets()));
                    }
                    candidate.pop_back();
                    // logg("CreateNewItemsets 1.1.1.2\n");
                }
            }
            candidate.pop_back();
            // logg("CreateNewItemsets 1.1.2\n");
        }
        // logg("CreateNewItemsets 1.2\n");
    }
    // logg("CreateNewItemsets 2\n");

    itemset.CreateNewLayer(new_items_info);
    // logg("CreateNewItemsets end\n");
}
}  // namespace algos::cind
