#pragma once

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

namespace model {

struct DCCandidate {
    boost::dynamic_bitset<> bitset{64};
    boost::dynamic_bitset<> cand{64};
};

}  // namespace model
