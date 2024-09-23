#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "algorithms/md/hymd/column_classifier_value_id.h"
#include "algorithms/md/hymd/table_identifiers.h"

namespace algos::hymd::preprocessing {
template <typename ResultType>
using ValueResult = std::pair<ResultType, ValueIdentifier>;
template <typename ResultType>
using ValidRowResults = std::vector<ValueResult<ResultType>>;
template <typename ResultType>
using RowInfo = std::pair<ValidRowResults<ResultType>, std::size_t>;
template <typename ResultType>
using ValidTableResults = std::vector<RowInfo<ResultType>>;

using EnumeratedValidTableResults = ValidTableResults<ColumnClassifierValueId>;
}  // namespace algos::hymd::preprocessing
