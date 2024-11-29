#pragma once

#include <random>

namespace algos::des {

class RNG {
private:
    int const kSeed = 2;
    std::mt19937 rng_{kSeed};
    std::uniform_real_distribution<double> uni_{0.0, 1.0};

public:
    double Next() {
        return uni_(rng_);
    }
};

}  // namespace algos::des
