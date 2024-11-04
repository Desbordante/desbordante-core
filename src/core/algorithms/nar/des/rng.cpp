#include "rng.h"

namespace algos::des {

int const kSeed = 2;
std::mt19937 RNG::rng_ = std::mt19937(kSeed);
std::uniform_real_distribution<double> RNG::uni_ = std::uniform_real_distribution(0.0, 1.0);

}  // namespace algos::des
