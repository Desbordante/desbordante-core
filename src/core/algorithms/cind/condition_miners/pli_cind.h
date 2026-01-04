#pragma once

#include <vector>

// #include "algorithms/ind/cind/condition.h"
#include "cind_miner.h"
#include "core/algorithms/cind/cind.h"
#include "position_lists_set.h"

namespace algos::cind {
using model::PLSet;
using PLSetShared = std::shared_ptr<model::PLSet>;

class PliCind final : public CindMiner {
private:
    std::vector<PLSetShared> attr_idx_to_pls_;
    size_t relation_size_;  // num of groups/rows

public:
    PliCind(config::InputTables& input_tables);

    std::vector<Condition> GetConditions(Attributes const& attrs);

private:
    CIND ExecuteSingle(model::IND const& aind) final;
    void MakePLs(Attributes const& attrs);

    std::pair<std::vector<int>, std::vector<int>> ClassifyRows(Attributes const& attrs);

    std::vector<Condition> Analyze(size_t attr_idx, std::vector<int> const& curr_attrs,
                                   PLSetShared const& curr_pls, AttrsType const& cond_attrs,
                                   std::vector<int> const& row_to_group,
                                   std::vector<int> const& included_pos);

    void Reset();
};

}  // namespace algos::cind