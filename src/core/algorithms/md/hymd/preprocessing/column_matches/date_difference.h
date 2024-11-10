#pragma once

#include <array>
#include <cstdlib>

#include "algorithms/md/hymd/preprocessing/column_matches/lv_normalized.h"
#include "config/exceptions.h"

namespace algos::hymd::preprocessing::column_matches {
inline size_t DateDifference(model::Date const& left, model::Date const& right) {
    return std::abs((left - right).days());
}

class LVNormDateDifference : public LVNormalized<DateDifference, true> {
    static constexpr auto kName = "date_difference";

public:
    template <typename... Args>
    LVNormDateDifference(Args&&... args)
        : LVNormalized<DateDifference, true>(kName, std::forward<Args>(args)...) {}
};

}  // namespace algos::hymd::preprocessing::column_matches
