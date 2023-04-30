#pragma once

#include <memory>

#include "algorithms/algorithm_with_data.h"
#include "model/idataset_stream.h"

namespace algos {
using RelationStream = std::shared_ptr<model::IDatasetStream>;
using RelationalAlgorithm = AlgorithmWithData<RelationStream>;
}  // namespace algos
