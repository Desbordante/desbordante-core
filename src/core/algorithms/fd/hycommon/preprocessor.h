#pragma once

#include <tuple>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/hycommon/types.h"
#include "core/model/table/idataset_stream.h"

namespace algos::hy::util {

std::vector<ClusterId> SortAndGetMapping(PLIs& plis);
Columns BuildInvertedPlis(PLIs const& plis);
Rows BuildRecordRepresentation(Columns const& inverted_plis);

}  // namespace algos::hy::util

namespace algos::hy {

std::tuple<std::shared_ptr<PLIs>, std::shared_ptr<Rows>, std::vector<ClusterId>> Preprocess(model::IDatasetStream& data_stream);
boost::dynamic_bitset<> RestoreAgreeSet(boost::dynamic_bitset<> const& as,
                                        std::vector<ClusterId> const& og_mapping, size_t num_cols);

}  // namespace algos::hy
