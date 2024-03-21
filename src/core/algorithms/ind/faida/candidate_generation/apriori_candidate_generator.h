#pragma once

#include "algorithms/ind/faida/util/simple_ind.h"

namespace algos::faida::apriori_candidate_generator {

std::vector<SimpleIND> CreateCombinedCandidates(std::vector<SimpleIND> const& inds);

}  // namespace algos::faida::apriori_candidate_generator
