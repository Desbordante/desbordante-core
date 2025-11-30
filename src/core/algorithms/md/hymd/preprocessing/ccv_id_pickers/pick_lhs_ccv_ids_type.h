#pragma once

#include <functional>
#include <vector>

#include "core/algorithms/md/hymd/column_classifier_value_id.h"
#include "core/algorithms/md/hymd/preprocessing/similarity.h"

namespace algos::hymd::preprocessing::ccv_id_pickers {
template <typename T>
using PickLhsCCVIdsType =
        std::function<std::vector<ColumnClassifierValueId>(std::vector<T> const&)>;

using SimilaritiesPicker = PickLhsCCVIdsType<Similarity>;
}  // namespace algos::hymd::preprocessing::ccv_id_pickers
