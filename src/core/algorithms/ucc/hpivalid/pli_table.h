#pragma once

#include <deque>
#include <limits>
#include <vector>

#include "algorithms/fd/hycommon/types.h"
#include "model/table/position_list_index.h"
#include "model/table/tuple_index.h"

// see algorithms/ucc/hpivalid/LICENSE

namespace algos::hpiv {

// a table in the form of PLIs together with the inverse mapping and
// some additional information
struct PLITable {
    // the PLIs: for each column, we have a vector of clusters where
    // each cluster is a vector of row IDs
    std::vector<std::deque<model::PLI::Cluster>> plis;

    // the inverse mapping: for each column, we have a vector of mapping
    // a row ID to a cluster ID
    algos::hy::Columns inverse_mapping;

    // the number of rows
    model::TupleIndex nr_rows;

    // the number of columns
    model::ColumnIndex nr_cols;
};

// cluster id to use for all clusters of size one
constexpr unsigned kSizeOneCluster = std::numeric_limits<unsigned>::max();

}  // namespace algos::hpiv
