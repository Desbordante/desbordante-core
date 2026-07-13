#pragma once

#include "gdd_validator.h"

namespace algos {

struct GddValidator::PatternGrouping {
    std::vector<model::Gdd const*> gdds;
    // gdds[i] was passed to the algorithm at index origin[i], we want to restore the initial order
    // of users gdds
    std::vector<std::size_t> origin;
    // group "g" occupies [starts[g], starts[g + 1]) in gdds
    std::vector<std::size_t> starts;

    std::size_t GroupCount() const noexcept {
        return starts.size() - 1;
    }

    static PatternGrouping GroupByPattern(std::vector<model::Gdd> const& gdds);
};

}  // namespace algos
