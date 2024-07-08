#pragma once

#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/indexes/similarity_index.h"
#include "algorithms/md/hymd/indexes/similarity_matrix.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"

namespace algos::hymd::indexes {
struct ColumnMatchSimilarityInfo {
    std::vector<preprocessing::Similarity> classifier_values;
    SimilarityMatrix similarity_matrix;
    SimilarityIndex similarity_index;
    // TODO: add slim similarity index
};

struct SimilarityMeasureOutput {
    std::vector<ColumnClassifierValueId> lhs_ids;
    ColumnMatchSimilarityInfo indexes;
};
}  // namespace algos::hymd::indexes
