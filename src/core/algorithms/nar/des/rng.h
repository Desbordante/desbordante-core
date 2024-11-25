#pragma once

#include <random>

namespace algos::des {

class RNG {
private:
    long unsigned const kSeed_ = 2;
    // result_type can differ on different STL implementations and data models
    std::mt19937 rng_{static_cast<std::mt19937::result_type>(kSeed_)};
    std::uniform_real_distribution<double> uni_{0.0, 1.0};

public:
    double Next() {
        return uni_(rng_);
    }
};

}  // namespace algos::des
