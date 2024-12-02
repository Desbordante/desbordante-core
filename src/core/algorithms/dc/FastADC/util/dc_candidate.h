#pragma once

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

#include "dc/FastADC/model/predicate.h"

namespace algos::fastadc {

struct DCCandidate {
    boost::dynamic_bitset<> bitset{64};
    boost::dynamic_bitset<> cand{64};
};

}  // namespace algos::fastadc
