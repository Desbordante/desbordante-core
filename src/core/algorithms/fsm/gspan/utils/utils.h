#pragma once

#include <vector>

#include <boost/unordered/unordered_flat_set.hpp>

#include "types/graph.h"

namespace gspan {

boost::unordered_flat_set<int> TranslateToOriginalIds(
        boost::unordered_flat_set<int> const& internal_ids, std::vector<graph_t> const& graph_db);

csr_graph_t ConvertToCSR(graph_t const& src_graph);

}  // namespace gspan