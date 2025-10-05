#pragma once

#include <list>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/cfd/cfdfinder/model/pattern/pattern.h"

namespace algos::cfdfinder {

class ExpansionStrategy {
public:
    virtual ~ExpansionStrategy() = default;
    virtual Pattern GenerateNullPattern(boost::dynamic_bitset<> const& attributes) = 0;
    virtual std::list<Pattern> GetChildPatterns(Pattern const& pattern) = 0;
};

}  // namespace algos::cfdfinder