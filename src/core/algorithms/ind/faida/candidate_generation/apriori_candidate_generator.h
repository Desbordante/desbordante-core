#pragma once

#include "algorithms/ind/faida/util/simple_ind.h"

namespace algos::faida::AprioriCandidateGenerator {

std::vector<SimpleIND> CreateCombinedCandidates(std::vector<SimpleIND> const& inds);

}  // namespace algos::faida::AprioriCandidateGenerator
