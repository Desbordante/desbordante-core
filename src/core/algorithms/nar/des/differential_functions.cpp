#include "differential_functions.h"

#include <algorithm>

namespace algos::des {

// gets slow if population ~= number_of_indices
std::vector<size_t> GetRandIndices(size_t except_index, size_t population, size_t number_of_indices,
                                   RNG& rng) {
    assert(number_of_indices <= population - 1);
    std::unordered_set<size_t> indices;
    indices.insert(except_index);
    while (indices.size() < number_of_indices + 1) {
        size_t random_index = rng.Next() * population;
        indices.insert(random_index);
    }
    indices.erase(except_index);
    std::vector<size_t> ind_vec;
    ind_vec.reserve(number_of_indices);
    ind_vec.insert(ind_vec.end(), indices.begin(), indices.end());
    return ind_vec;
}

EncodedNAR Rand1Bin(std::vector<EncodedNAR> const& population, size_t candidate_index,
                    DifferentialOptions options, RNG& rng) {
    auto sample_indices = GetRandIndices(candidate_index, population.size(), 3, rng);
    size_t sample_index1 = sample_indices[0];
    size_t sample_index2 = sample_indices[1];
    size_t sample_index3 = sample_indices[2];

    auto new_individual = population[candidate_index];
    auto sample1 = population[sample_index1];
    auto sample2 = population[sample_index2];
    auto sample3 = population[sample_index3];

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