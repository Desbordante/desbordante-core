#pragma once

#include <unordered_set>
#include <vector>

#include "model/table/tuple_index.h"

namespace algos::od {

// Represents a minimal set of rows which should be removed from a table in order for exact OD to
// hold. Uses std::vector to hold tuple indices, it should be guaranteed that all indices in vector
// are unique.
using RemovalSetAsVec = std::vector<model::TupleIndex>;
using RemovalSet = std::unordered_set<model::TupleIndex>;

}  // namespace algos::od
