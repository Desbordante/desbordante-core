#pragma once

#include <cstddef>

#include "core/algorithms/nar/des/encoded_nar.h"
#include "core/algorithms/nar/des/enums.h"
#include "core/algorithms/nar/des/rng.h"
#include "core/util/better_enum_with_visibility.h"

namespace algos::des {

struct DifferentialOptions {
    double differential_scale;
    double crossover_probability;
    DifferentialStrategy differential_strategy = DifferentialStrategy::best1Exp;
};

EncodedNAR Rand1Bin(std::vector<EncodedNAR> const& population, std::size_t candidate_index,
                    DifferentialOptions const& options, RNG& rng);

using MutationFunction = EncodedNAR (*)(std::vector<EncodedNAR> const& population,
                                        std::size_t candidate_index, DifferentialOptions const& options,
                                        RNG& rng);

MutationFunction EnumToMutationStrategy(DifferentialStrategy strategy);

}  // namespace algos::des
