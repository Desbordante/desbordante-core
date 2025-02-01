#pragma once

#include <vector>

// #include "algorithms/ind/cind/condition.h"
#include "ind/cind/condition_miners/cind_miner.hpp"
#include "table/encoded_column_data.h"

// #include "position_lists_set.h"

namespace algos::cind {
using model::EncodedColumnData;
// using model::PLSet;
// using PLSetShared = std::shared_ptr<model::PLSet>;
using AttrsType = std::vector<EncodedColumnData const*>;

class PliCind final : public CindMiner {
    // private:
    //     std::vector<PLSetShared> attr_idx_to_pls_;

public:
    PliCind(config::InputTables& input_tables);

    // std::unordered_set<Condition> GetConditions(std::vector<int> cond_attrs,
    //                                             std::vector<int> const& included_pos);

private:
    void ExecuteSingle(model::IND const& aind) final;
    void MakePLs(std::vector<int> const& cond_attrs);

    std::pair<std::vector<int>, AttrsType> ScanDomains(model::IND const& aind) const;

    // std::unordered_set<Condition> Analyze(std::vector<int> const& cond_attrs, int attr_idx,
    //                                       std::vector<int> const& curr_attrs, PLSetShared
    //                                       curr_pls, std::vector<int> const& included_pos);
};

}  // namespace algos::cind