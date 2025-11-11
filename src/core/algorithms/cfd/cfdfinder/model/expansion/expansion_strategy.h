#pragma once

#include <list>

#include "algorithms/cfd/cfdfinder/model/pattern/pattern.h"
#include "algorithms/cfd/cfdfinder/types/bitset.h"

namespace algos::cfdfinder {

class ExpansionStrategy {
public:
    virtual ~ExpansionStrategy() = default;
    virtual Pattern GenerateNullPattern(BitSet const& attributes) = 0;
    virtual std::list<Pattern> GetChildPatterns(Pattern const& pattern) = 0;
};

}  // namespace algos::cfdfinder