#pragma once

#include <vector>

#include "basket.h"
#include "cind/cind.h"
#include "cind/condition_miners/cind_miner.h"
#include "itemset.h"

namespace algos::cind {
class Cinderella final : public CindMiner {
public:
    explicit Cinderella(config::InputTables& input_tables);

private:
    CIND ExecuteSingle(model::IND const& aind) final;

    std::vector<Basket> GetBaskets(Attributes const& attributes);

    std::vector<Condition> GetConditions(std::vector<Basket> const& baskets,
                                         AttrsType const& condition_attrs) const;

    void CreateNewItemsets(Itemset& itemset) const;
};
}  // namespace algos::cind
