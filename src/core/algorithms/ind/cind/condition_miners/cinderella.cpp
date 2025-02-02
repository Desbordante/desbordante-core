#include "cinderella.h"

#include <map>

#include "ind/cind/condition_miners/basket.h"

namespace algos::cind {
Cinderella::Cinderella(config::InputTables& input_tables) : CindMiner(input_tables) {}

void Cinderella::ExecuteSingle(model::IND const& aind) {
    auto baskets = GetBaskets(aind);
    for (auto const& basket : baskets) {
        fprintf(stderr, "basket: [%s, (", basket.is_included ? "included" : "null");
        for (auto const& item : basket.items) {
            fprintf(stderr, "{%u, %s, %s}, ", item.table_id,
                    tables_[item.table_id]
                            ->GetColumnData(item.column_id)
                            .GetColumn()
                            ->GetName()
                            .c_str(),
                    tables_[item.table_id]
                            ->GetColumnData(item.column_id)
                            .DecodeValue(item.value)
                            .c_str());
        }
        fprintf(stderr, ")]\n");
    }
    fprintf(stderr, "\n");
}

std::vector<Basket> Cinderella::GetBaskets(model::IND const& aind) {
    auto attributes = ClassifyAttributes(aind);
    std::set<std::vector<std::string>> rhs_values;
    std::map<std::vector<std::string>, std::unordered_set<Item>> items_by_value;

    fprintf(stderr, "rhs values: [");
    for (size_t index = 0; index < attributes.rhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<std::string> row;
        fprintf(stderr, "{");
        for (auto& attr : attributes.rhs_inclusion) {
            row.push_back(attr->GetStringValue(index));
            fprintf(stderr, "%s, ", attr->GetStringValue(index).c_str());
        }
        fprintf(stderr, "}, ");
        rhs_values.insert(std::move(row));

        for (auto const& cond_attr : attributes.conditional) {
            if (cond_attr->GetTableId() == attributes.rhs_inclusion.back()->GetTableId()) {
                items_by_value[row].insert({.table_id = cond_attr->GetTableId(),
                                            .column_id = cond_attr->GetColumnId(),
                                            .value = cond_attr->GetValue(index)});
            }
        }
    }
    fprintf(stderr, "]\n");

    std::vector<Basket> result;
    std::map<std::vector<std::string>, Basket*> baskets_by_value;
    fprintf(stderr, "lhs values: [");
    for (size_t index = 0; index < attributes.lhs_inclusion.front()->GetNumRows(); ++index) {
        std::vector<std::string> row;
        fprintf(stderr, "{");
        for (auto& attr : attributes.lhs_inclusion) {
            row.push_back(attr->GetStringValue(index));
            fprintf(stderr, "%s, ", attr->GetStringValue(index).c_str());
        }
        fprintf(stderr, "}");
        if (auto const& it = baskets_by_value.find(row); it == baskets_by_value.cend()) {
            result.push_back({.is_included = rhs_values.contains(row),
                              .items = std::move(items_by_value[row])});
            baskets_by_value[row] = &result[result.size() - 1];
        }
        for (auto const& cond_attr : attributes.conditional) {
            if (cond_attr->GetTableId() == attributes.lhs_inclusion.back()->GetTableId()) {
                baskets_by_value[row]->items.insert({.table_id = cond_attr->GetTableId(),
                                                     .column_id = cond_attr->GetColumnId(),
                                                     .value = cond_attr->GetValue(index)});
            }
        }
        if (rhs_values.contains(row)) {
            fprintf(stderr, "::included");
        }
        fprintf(stderr, ", ");
    }
    fprintf(stderr, "]\n");
    return result;
}

void Cinderella::GetFrequentItemsets(std::vector<Basket> /*baskets*/) {
    // std::vector<Itemset> curr_itemsets;
    // for (auto const& basket : baskets) {
    //     if (!basket.is_included) continue;
    //     for (auto const& item : basket.items) {
    //         curr_itemsets.emplace_back(std::vector<int>{kIndicatorPos, item.column_id},
    //                                    std::vector<int>{(int)Indicator::kIncluded, item.value});
    //     }
    // }

    // for (size_t k = 3; !curr_itemsets.empty(); ++k) {
    //     auto candidates = GetCandidates(curr_itemsets);
    //     for (auto const& basket : baskets) {
    //         auto valid_for_basket =
    //                 Subset(basket, candidates) for (auto& candidate : valid_for_basket) {
    //             candidate.count++;
    //         }
    //     }
    //     curr_itemsets.clear();
    //     for (auto& candidate : valid_for_basket) {
    //         if (double recall = (double)candidate.count / baskets.size(); recall > recall_) {
    //             curr_itemsets.push_back(std::move(candidate));
    //         }
    //     }
    // }
}
}  // namespace algos::cind