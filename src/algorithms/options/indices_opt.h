#pragma once

#include <vector>

#include "algorithms/options/descriptions.h"
#include "algorithms/options/names.h"
#include "algorithms/options/type.h"

namespace algos::config {

using IndexType = unsigned int;
using IndicesType = std::vector<IndexType>;

void TransformIndices(IndicesType& value);

void ValidateIndex(IndexType value, size_t cols_count);

void ValidateIndices(IndicesType const& value, size_t cols_count);

const OptionType<IndexType> RhsIndexOpt{
        {config::names::kRhsIndex, config::descriptions::kDRhsIndex}};

const OptionType<IndicesType> LhsIndicesOpt{
        {config::names::kLhsIndices, config::descriptions::kDLhsIndices}, {}, TransformIndices};

const OptionType<IndicesType> RhsIndicesOpt{
        {config::names::kRhsIndices, config::descriptions::kDRhsIndices}, {}, TransformIndices};

}  // namespace algos::config
