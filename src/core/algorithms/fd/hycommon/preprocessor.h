#pragma once

#include <tuple>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/hycommon/types.h"
#include "core/model/table/column_layout_relation_data.h"

namespace algos::hy::util {

std::vector<ClusterId> SortAndGetMapping(PLIs& plis);
Columns BuildInvertedPlis(PLIs const& plis);
Rows BuildRecordRepresentation(Columns const& inverted_plis);
PLIs BuildPLIs(ColumnLayoutRelationData* relation);

}  // namespace algos::hy::util

namespace algos::hy {

std::tuple<PLIs, Rows, std::vector<ClusterId>> Preprocess(ColumnLayoutRelationData* relation);
boost::dynamic_bitset<> RestoreAgreeSet(boost::dynamic_bitset<> const& as,
                                        std::vector<ClusterId> const& og_mapping, size_t num_cols);

}  // namespace algos::hy
