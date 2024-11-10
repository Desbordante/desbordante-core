#pragma once

#include <cstddef>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/indexes/similarity_index.h"
#include "algorithms/md/hymd/indexes/similarity_matrix.h"
#include "algorithms/md/hymd/lhs_ccv_ids_info.h"
#include "algorithms/md/hymd/lowest_cc_value_id.h"
#include "algorithms/md/hymd/preprocessing/similarity.h"

namespace algos::hymd::indexes {
struct ColumnMatchSimilarityInfo {
    std::vector<preprocessing::Similarity> classifier_values;
    SimilarityMatrix similarity_matrix;
    SimilarityIndex similarity_index;
};

struct ColumnPairMeasurements {
    LhsCCVIdsInfo lhs_ccv_ids_info;
    ColumnMatchSimilarityInfo indexes;

    static LhsCCVIdsInfo MakeLhsCCVIdsInfo(std::vector<ColumnClassifierValueId>&& lhs_ids,
                                           ColumnMatchSimilarityInfo const& indexes) {
        std::vector<ColumnClassifierValueId> cm_map;
        std::size_t const values_size = indexes.classifier_values.size();
        cm_map.reserve(values_size);
        auto next = lhs_ids.begin(), prev = next++, end = lhs_ids.end();
        cm_map.insert(cm_map.end(), lhs_ids.front(), kLowestCCValueId);
        model::Index cur_i = 0;
        for (; next != end; ++prev, ++next, ++cur_i) {
            cm_map.insert(cm_map.end(), *next - *prev, cur_i);
        }
        cm_map.insert(cm_map.end(), values_size - *prev, cur_i);
        return {std::move(lhs_ids), std::move(cm_map)};
    }

    ColumnPairMeasurements(std::vector<ColumnClassifierValueId> lhs_ids,
                           ColumnMatchSimilarityInfo indexes)
        : lhs_ccv_ids_info(MakeLhsCCVIdsInfo(std::move(lhs_ids), indexes)),
          indexes(std::move(indexes)) {}
};
}  // namespace algos::hymd::indexes
