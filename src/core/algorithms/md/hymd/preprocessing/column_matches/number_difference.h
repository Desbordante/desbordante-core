#pragma once

#include "core/algorithms/md/hymd/preprocessing/column_matches/lv_normalized.h"
#include "core/config/exceptions.h"

namespace algos::hymd::preprocessing::column_matches {
inline double NumberDifference(model::Double left, model::Double right) {
    return std::abs(left - right);
}

class LVNormNumberDistance : public LVNormalized<NumberDifference, true> {
    static constexpr auto kName = "number_difference";

public:
    template <typename... Args>
    LVNormNumberDistance(Args&&... args)
        : LVNormalized<NumberDifference, true>(kName, std::forward<Args>(args)...) {}
};
}  // namespace algos::hymd::preprocessing::column_matches
