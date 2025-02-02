#pragma once

#include <vector>

#include "basket.h"
#include "itemset.h"
#include "ind/cind/condition_miners/cind_miner.hpp"

enum class Indicator : int { kNull = 0, kIncluded = 1 };
int const kIndicatorPos = -1;

namespace algos::cind {
class Cinderella final : public algos::cind::CindMiner {
public:
    Cinderella(config::InputTables& input_tables);

private:
    void ExecuteSingle(model::IND const& aind) final;

    std::vector<Basket> GetBaskets(model::IND const& aind);

    void GetFrequentItemsets(std::vector<Basket> baskets);
    std::vector<Itemset> GetCandidates(std::vector<Itemset> const& curr_itemsets);

private:
    double precision_;
    double recall_;
};
}  // namespace algos::cind
