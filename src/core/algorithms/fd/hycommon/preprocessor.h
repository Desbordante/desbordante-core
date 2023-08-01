#pragma once

#include <tuple>

#include <boost/dynamic_bitset.hpp>

#include "model/table/column_layout_relation_data.h"
#include "types.h"

namespace algos::hy {

std::tuple<PLIs, Rows, std::vector<ClusterId>> Preprocess(ColumnLayoutRelationData* relation);
boost::dynamic_bitset<> RestoreAgreeSet(boost::dynamic_bitset<> const& as,
                                        std::vector<ClusterId> const& og_mapping, size_t num_cols);

}  // namespace algos::hy
