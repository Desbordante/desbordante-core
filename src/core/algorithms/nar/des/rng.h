#pragma once

#include <random>

namespace algos::des {

class RNG {
private:
    long unsigned const kSeed_ = 2;
    std::mt19937 rng_{kSeed_};
    std::uniform_real_distribution<double> uni_{0.0, 1.0};

public:
    double Next() {
        return uni_(rng_);
    }
};

}  // namespace algos::des
