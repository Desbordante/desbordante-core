#pragma once

#include <vector>

#include "basket.h"
#include "cind/condition_miners/cind_miner.hpp"
#include "itemset.h"

namespace algos::cind {
class Cinderella final : public algos::cind::CindMiner {
public:
    Cinderella(config::InputTables& input_tables);

private:
    void ExecuteSingle(model::IND const& aind) final;

    std::vector<Basket> GetBaskets(model::IND const& aind);

    std::vector<Itemset> GetFrequentItemsets(std::vector<Basket> const& baskets) const;
    std::set<Itemset> CreateNewItemsets(std::set<Itemset> candidates,
                                        std::vector<Basket> const& baskets,
                                        int included_baskets_cnt,
                                        std::vector<Itemset>& result) const;

    std::set<Itemset> GetCandidates(std::set<Itemset> const& curr_itemsets) const;
};
}  // namespace algos::cind
