#pragma once

#include <vector>

#include "basket.h"
#include "cind/cind.hpp"
#include "cind/condition_miners/cind_miner.hpp"
#include "itemset.h"

namespace algos::cind {
class Cinderella final : public algos::cind::CindMiner {
public:
    Cinderella(config::InputTables& input_tables);

private:
    Cind ExecuteSingle(model::IND const& aind) final;

    std::vector<Basket> GetBaskets(Attributes const& attributes);

    std::vector<Condition> GetConditions(std::vector<Basket> const& baskets,
                                         AttrsType const& condition_attrs) const;
    std::set<Itemset> CreateNewItemsets(std::set<Itemset> candidates,
                                        std::vector<Basket> const& baskets,
                                        int included_baskets_cnt, AttrsType const& condition_attrs,
                                        std::vector<Condition>& result) const;

    std::set<Itemset> GetCandidates(std::set<Itemset> const& curr_itemsets) const;
};
}  // namespace algos::cind
