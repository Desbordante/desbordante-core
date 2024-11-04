#pragma once

#include <random>

namespace algos::des {

class RNG {
private:
    static std::mt19937 rng_;
    static std::uniform_real_distribution<double> uni_;

public:
    static double Next() {
        return uni_(rng_);
    }
};

}  // namespace algos::des
