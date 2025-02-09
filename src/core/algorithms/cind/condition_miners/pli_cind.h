#pragma once

#include <vector>

// #include "algorithms/ind/cind/condition.h"
#include "cind/cind.hpp"
#include "cind/condition_miners/cind_miner.hpp"

// #include "position_lists_set.h"

namespace algos::cind {
// using model::PLSet;
// using PLSetShared = std::shared_ptr<model::PLSet>;

class PliCind final : public CindMiner {
    // private:
    //     std::vector<PLSetShared> attr_idx_to_pls_;

public:
    PliCind(config::InputTables& input_tables);

    // std::unordered_set<Condition> GetConditions(std::vector<int> cond_attrs,
    //                                             std::vector<int> const& included_pos);

private:
    Cind ExecuteSingle(model::IND const& aind) final;
    void MakePLs(std::vector<int> const& cond_attrs);

    std::vector<int> GetIncludedPositions(const Attributes & attrs) const;

    std::pair<std::vector<int>, AttrsType> ScanTables(model::IND const& aind) const;

    // std::unordered_set<Condition> Analyze(std::vector<int> const& cond_attrs, int attr_idx,
    //                                       std::vector<int> const& curr_attrs, PLSetShared
    //                                       curr_pls, std::vector<int> const& included_pos);
};

}  // namespace algos::cind