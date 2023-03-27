#pragma once

#include <vector>

#include "algorithms/options/common_option.h"
#include "algorithms/options/names_and_descriptions.h"

namespace algos::config {

using IndexType = unsigned int;
using IndicesType = std::vector<IndexType>;

void TransformIndices(IndicesType& value);

void ValidateIndex(IndexType value, size_t cols_count);

void ValidateIndices(IndicesType const& value, size_t cols_count);

const CommonOption<IndexType> RhsIndexOpt{
        {config::names::kRhsIndex, config::descriptions::kDRhsIndex}};

const CommonOption<IndicesType> LhsIndicesOpt{
        {config::names::kLhsIndices, config::descriptions::kDLhsIndices}, {}, TransformIndices};

const CommonOption<IndicesType> RhsIndicesOpt{
        {config::names::kRhsIndices, config::descriptions::kDRhsIndices}, {}, TransformIndices};

}  // namespace algos::config
