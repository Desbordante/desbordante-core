#pragma once

#include <utility>
#include <vector>

#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::record_match_indexes::calculators {
// A result is meaningful if RCV ID != 0 or the decision boundary is not universal.
template <typename ComparisonResultType>
using MeaningfulResult = std::pair<ComparisonResultType, PartitionValueId /*right part. value*/>;
template <typename ComparisonResultType>
using MeaningfulLeftValueResults = std::vector<MeaningfulResult<ComparisonResultType>>;
// TODO: use structs
template <typename ComparisonResultType>
using LeftValueComparisonInfo = std::pair<MeaningfulLeftValueResults<ComparisonResultType>,
                                          /*number of records*/ std::size_t>;
template <typename ComparisonResultType>
using MeaningfulDataResults = std::vector<LeftValueComparisonInfo<ComparisonResultType>>;

using EnumeratedMeaningfulDataResults = MeaningfulDataResults<RecordClassifierValueId>;
}  // namespace algos::hymde::record_match_indexes::calculators
