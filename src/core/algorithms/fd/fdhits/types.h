#pragma once

#include <cstddef>
#include <deque>
#include <vector>

#include "core/algorithms/fd/fdhits/validator/cluster_filter.h"
#include "core/model/table/position_list_index.h"

namespace algos::fd::fdhits {

using RowIndex = std::size_t;
using ColumnIndex = std::size_t;
using ClusterIndex = unsigned int;

using Pli = std::deque<Cluster>;

}  // namespace algos::fd::fdhits
