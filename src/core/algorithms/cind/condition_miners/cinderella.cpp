// cinderella.cpp
#include "cinderella.h"

#include <algorithm>
#include <iterator>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/container_hash/hash.hpp>

#include "cind/condition.h"
#include "cind/condition_miners/basket.h"
#include "cind/types.h"
#include "itemset_node.h"

namespace algos::cind {
namespace {

struct VectorIntHash {
    std::size_t operator()(std::vector<int> const& vec) const noexcept {
        return boost::hash_value(vec);
    }
};

std::vector<BasketInfo> MergeBaskets(std::vector<BasketInfo> const& baskets1,
                                     std::vector<BasketInfo> const& baskets2) {
    std::size_t i1 = 0;
    std::size_t i2 = 0;

    std::vector<BasketInfo> result;
    result.reserve(std::min(baskets1.size(), baskets2.size()));

    while (i1 < baskets1.size() && i2 < baskets2.size()) {
        auto const& [index1, positions1, is_included1] = baskets1[i1];
        auto const& [index2, positions2, is_included2] = baskets2[i2];

        if (index1 < index2) {
            ++i1;
        } else if (index1 > index2) {
            ++i2;
        } else {
            std::vector<size_t> positions_intersection;
            positions_intersection.reserve(std::min(positions1.size(), positions2.size()));

            std::set_intersection(positions1.begin(), positions1.end(), positions2.begin(),
                                  positions2.end(), std::back_inserter(positions_intersection));

            if (!positions_intersection.empty()) {
                result.emplace_back(index1, std::move(positions_intersection), is_included1);
            }
            ++i1;
            ++i2;
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
    std::unordered_set<std::vector<int>, VectorIntHash> rhs_values;

    for (size_t index = 0; index < attributes.rhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<int> row;
        row.reserve(attributes.rhs_inclusion.size());
        for (auto& attr : attributes.rhs_inclusion) {
            row.push_back(attr->GetValue(index));
        }
        rhs_values.insert(std::move(row));
    }

    std::vector<Basket> result;
    std::unordered_map<std::vector<int>, std::size_t, VectorIntHash> basket_id_by_value;

    for (size_t index = 0; index < attributes.lhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<int> row;
        row.reserve(attributes.lhs_inclusion.size());
        for (auto& attr : attributes.lhs_inclusion) {
            row.push_back(attr->GetValue(index));
        }

        if (condition_type_._value == CondType::group) {
            auto it = basket_id_by_value.find(row);
            if (it == basket_id_by_value.end()) {
                bool const included = rhs_values.contains(row);
                std::size_t const id = result.size();

                result.emplace_back(algos::cind::Basket{included, ItemsInfo{}});
                it = basket_id_by_value.emplace(row, id).first;
            }

            auto const basket_id = it->second;
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
            result.emplace_back(algos::cind::Basket{rhs_values.contains(row), std::move(basket_items)});
        }
    }
    return result;
}

std::vector<Condition> Cinderella::GetConditions(std::vector<Basket> const& baskets,
                                                 AttrsType const& condition_attrs) const {
    std::unordered_map<Item, std::vector<BasketInfo>> first_level_items;

    // scan all included baskets to extract all included items
    // number of all included baskets - needed for computing completeness of conditions
    std::size_t included_baskets_cnt = 0;
    for (auto const& basket : baskets) {
        if (!basket.is_included) {
            continue;
        }
        ++included_baskets_cnt;
        for (auto const& [item, _] : basket.items) {
            first_level_items.try_emplace(item, std::vector<BasketInfo>{});
        }
    }

    for (size_t basket_id = 0; basket_id < baskets.size(); ++basket_id) {
        auto const& basket = baskets.at(basket_id);
        for (auto const& [item, positions] : basket.items) {
            if (auto it = first_level_items.find(item); it != first_level_items.end()) {
                it->second.emplace_back(basket_id, positions, basket.is_included);
            }
        }
    }

    // result stores all frequent and valid itemsets
    std::vector<Condition> result;

    Itemset itemset(std::move(first_level_items), included_baskets_cnt, min_completeness_);
    while (!itemset.GetItems().empty()) {
        for (auto const& candidate : itemset.GetItems()) {
            if (candidate->GetValidity() >= min_validity_) {
                result.emplace_back(candidate, condition_attrs);
            }
        }
        CreateNewItemsets(itemset);
    }
    return result;
}

void Cinderella::CreateNewItemsets(Itemset& itemset) const {
    std::vector<std::tuple<std::shared_ptr<ItemsetNode>, Item, std::vector<BasketInfo>>> new_items_info;

    for (auto const& itemset_prefix : itemset.GetPrevItems()) {
        std::vector<Item> candidate = itemset_prefix->GetContents();

        for (auto const& [_, parent] : itemset_prefix->GetChildNodes()) {
            candidate.push_back(parent->GetValue());

            for (auto const& [__, elem] : itemset_prefix->GetChildNodes()) {
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
