#pragma once

#include <boost/dynamic_bitset.hpp>

#include "dc/FastADC/model/predicate.h"

namespace algos::fastadc {

struct DCCandidate {
    boost::dynamic_bitset<> bitset{kPredicateBits};
    boost::dynamic_bitset<> cand{kPredicateBits};
};

}  // namespace algos::fastadc
