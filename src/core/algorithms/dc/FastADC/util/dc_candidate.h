#pragma once

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

namespace algos::fastadc {

struct DCCandidate {
    boost::dynamic_bitset<> bitset{kPredicateBits};
    boost::dynamic_bitset<> cand{kPredicateBits};
};

}  // namespace algos::fastadc
