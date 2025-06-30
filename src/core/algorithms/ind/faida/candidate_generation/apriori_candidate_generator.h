#pragma once

#include <vector>  // for vector

#include "algorithms/ind/faida/util/simple_ind.h"  // for SimpleIND

namespace algos::faida::apriori_candidate_generator {

std::vector<SimpleIND> CreateCombinedCandidates(std::vector<SimpleIND> const& inds);

}  // namespace algos::faida::apriori_candidate_generator
