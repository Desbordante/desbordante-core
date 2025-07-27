#pragma once

#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::record_match_indexes {
using ValueMatrixRow =
        boost::unordered::unordered_flat_map<PartitionValueId, RecordClassifierValueId>;
using ValueMatrix = std::vector<ValueMatrixRow>;
}  // namespace algos::hymde::record_match_indexes
