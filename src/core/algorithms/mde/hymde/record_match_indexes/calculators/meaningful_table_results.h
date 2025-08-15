#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "algorithms/mde/hymde/partition_value_identifier.h"
#include "algorithms/mde/hymde/record_classifier_value_id.h"

namespace algos::hymde::record_match_indexes::calculators {
// A result is meaningful if RCV ID != 0 or the decision boundary is not universal.
template <typename ComparisonResultType>
struct MeaningfulResult {
    ComparisonResultType comparison_result;
    PartitionValueId rt_pvalue_id;
    std::size_t cluster_size;
};

template <typename ComparisonResultType>
using MeaningfulLeftValueResults = std::vector<MeaningfulResult<ComparisonResultType>>;
template <typename ComparisonResultType>
using MeaningfulDataResults = std::vector<MeaningfulLeftValueResults<ComparisonResultType>>;

using EnumeratedMeaningfulDataResults = MeaningfulDataResults<RecordClassifierValueId>;
}  // namespace algos::hymde::record_match_indexes::calculators
