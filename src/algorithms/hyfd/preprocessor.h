#pragma once

#include <tuple>

#include <boost/dynamic_bitset.hpp>

#include "model/column_layout_relation_data.h"
#include "types.h"

namespace algos::hyfd {

std::tuple<PLIs, Rows, std::vector<size_t>> Preprocess(ColumnLayoutRelationData *relation);
boost::dynamic_bitset<> RestoreAgreeSet(boost::dynamic_bitset<> const& as,
                                        std::vector<size_t> const& og_mapping, size_t num_cols);

}  // namespace algos::hyfd
