#pragma once

#include <list>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern.h"

namespace algos::cfdfinder {

class ExpansionStrategy {
public:
    using BitSet = boost::dynamic_bitset<>;

    virtual ~ExpansionStrategy() = default;
    virtual Pattern GenerateNullPattern(BitSet const& attributes) const = 0;
    virtual std::list<Pattern> GetChildPatterns(Pattern const& pattern) const = 0;
};

}  // namespace algos::cfdfinder
