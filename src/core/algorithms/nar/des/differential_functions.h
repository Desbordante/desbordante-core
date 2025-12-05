#pragma once

#include "encoded_nar.h"
#include "enums.h"
#include "rng.h"

namespace algos::des {

struct DifferentialOptions {
    double differential_scale;
    double crossover_probability;
    DifferentialStrategy differential_strategy = DifferentialStrategy::kBest1Exp;
};

EncodedNAR Rand1Bin(std::vector<EncodedNAR> const& population, size_t candidate_index,
                    DifferentialOptions const& options, RNG& rng);

using MutationFunction = EncodedNAR (*)(std::vector<EncodedNAR> const& population,
                                        size_t candidate_index, DifferentialOptions const& options,
                                        RNG& rng);

MutationFunction EnumToMutationStrategy(DifferentialStrategy strategy);

}  // namespace algos::des
