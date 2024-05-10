#pragma once

#include <chrono>

// see algorithms/ucc/hpivalid/LICENSE

namespace algos::hpiv {

struct Config {
    // exponent for determining the number of sampled row pairs
    double sample_exponent = 0.3;

    // initial seed for the random number generator
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    // whether or not to use the tiebreaker heuristic
    bool tiebreaker_heuristic = true;
};

}  // namespace algos::hpiv
