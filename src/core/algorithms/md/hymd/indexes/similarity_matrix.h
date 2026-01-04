#pragma once

#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

#include "core/algorithms/md/hymd/column_classifier_value_id.h"
#include "core/algorithms/md/hymd/table_identifiers.h"

namespace algos::hymd::indexes {
using SimilarityMatrixRow =
        boost::unordered::unordered_flat_map<ValueIdentifier, ColumnClassifierValueId>;
using SimilarityMatrix = std::vector<SimilarityMatrixRow>;
}  // namespace algos::hymd::indexes
