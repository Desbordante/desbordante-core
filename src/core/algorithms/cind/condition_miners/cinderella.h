#pragma once

#include <vector>

#include "basket.h"
#include "cind/cind.h"
#include "cind/condition_miners/cind_miner.h"
#include "itemset.h"

namespace algos::cind {
class Cinderella final : public algos::cind::CindMiner {
public:
    Cinderella(config::InputTables& input_tables);

private:
    CIND ExecuteSingle(model::IND const& aind) final;

    std::vector<Basket> GetBaskets(Attributes const& attributes);

    std::vector<Condition> GetConditions(std::vector<Basket> const& baskets,
                                         AttrsType const& condition_attrs) const;
    std::unordered_set<Itemset> CreateNewItemsets(std::unordered_set<Itemset> candidates,
                                        std::vector<Basket> const& baskets,
                                        int included_baskets_cnt, AttrsType const& condition_attrs,
                                        std::vector<Condition>& result) const;

    std::unordered_set<Itemset> GetCandidates(std::unordered_set<Itemset> const& curr_itemsets) const;
};
}  // namespace algos::cind
