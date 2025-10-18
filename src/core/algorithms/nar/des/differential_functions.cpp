#include "differential_functions.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <stdexcept>

#include "nar/des/encoded_nar.h"
#include "nar/des/enums.h"
#include "nar/des/rng.h"

namespace algos::des {

// gets slow if population ~= number_of_indices
std::vector<size_t> GetRandIndices(size_t except_index, size_t population, size_t number_of_indices,
                                   RNG& rng) {
    assert(number_of_indices <= population - 1);
    std::set<size_t> indices;
    indices.insert(except_index);
    while (indices.size() < number_of_indices + 1) {
        size_t random_index = rng.Next() * population;
        indices.insert(random_index);
    }
    indices.erase(except_index);
    return {indices.begin(), indices.end()};
}

EncodedNAR Rand1Bin(std::vector<EncodedNAR> const& population, size_t candidate_index,
                    DifferentialOptions const& options, RNG& rng) {
    auto sample_indices = GetRandIndices(candidate_index, population.size(), 3, rng);
    auto new_individual = population[candidate_index];
    auto sample1 = population[sample_indices[0]];
    auto sample2 = population[sample_indices[1]];
    auto sample3 = population[sample_indices[2]];

    for (size_t i = 0; i < new_individual.VectorSize(); ++i) {
        if (rng.Next() < options.crossover_probability) {
            double new_feature_val =
                    sample1[i] + options.differential_scale * (sample2[i] - sample3[i]);
            new_feature_val = std::clamp(new_feature_val, 0.0, 1.0);
            new_individual[i] = new_feature_val;
        }
    }
    return new_individual;
}

// TODO: name is probably inconsistent with how it's called in the field.
MutationFunction EnumToMutationStrategy(DifferentialStrategy strategy) {
    switch (strategy) {
        case DifferentialStrategy::rand1Bin:
            return Rand1Bin;
        default:
            throw std::logic_error("No mutation function corresponding to DifferentialStategy.");
    }
}

}  // namespace algos::des
