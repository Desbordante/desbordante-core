#pragma once

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include "core/algorithms/dc/FastADC/model/predicate.h"

namespace algos::fastadc {

struct DCCandidate {
    boost::dynamic_bitset<> bitset{kMaxPredicateBits};
    boost::dynamic_bitset<> cand{kMaxPredicateBits};
};

}  // namespace algos::fastadc
