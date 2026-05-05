#pragma once

#include <sys/types.h>

#include "core/algorithms/cind/condition_miners/cind_miner.h"

namespace algos::cind {

class CureCind final : public CindMiner {
private:
    friend class CindAlgorithm;

    uint min_support_{2};

    struct CureAttributes {
        AttrsType lhs_inclusion, rhs_inclusion, lhs_conditional, rhs_conditional;
    };

    struct PatternPair {
        std::size_t lhs_attr_idx;
        int lhs_value;
        std::size_t rhs_attr_idx;
        int rhs_value;
        std::size_t support;
    };

    CIND ExecuteSingle(model::IND const& aind) final;

    CureAttributes ClassifyCureAttributes(model::IND const& aind) const;

    std::vector<PatternPair> DiscoverPatterns(CureAttributes const& attrs);

    std::vector<Condition> MinimalCover(std::vector<PatternPair> const& patterns,
                                        CureAttributes const& attrs);

public:
    explicit CureCind(config::InputTables& input_tables);
};

}  // namespace algos::cind
