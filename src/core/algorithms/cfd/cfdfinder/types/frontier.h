#pragma once

#include <set>
#include <utility>

#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern.h"
#include "core/algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"

namespace algos::cfdfinder {

class Frontier {
private:
    std::multiset<Pattern, std::greater<Pattern>> container_;

public:
    void Emplace(Pattern&& pattern);

    Pattern Poll();

    bool Empty() const;

    void Rebuild(boost::dynamic_bitset<> const& used_rows_mask, Row const& inverted_pli_rhs,
                 PruningStrategy const& pruning_strategy);
};

}  // namespace algos::cfdfinder
