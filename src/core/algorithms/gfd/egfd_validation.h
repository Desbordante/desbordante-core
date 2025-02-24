#pragma once

#include <map>      // for map
#include <set>      // for set
#include <utility>  // for pair
#include <vector>   // for vector

#include <boost/iterator/iterator_facade.hpp>  // for operator!=

#include "algorithms/gfd/gfd_handler.h"  // for GfdHandler
#include "gfd.h"                         // for Gfd
#include "gfd/graph_descriptor.h"        // for vertex_t, graph_t

namespace algos {

using CPI = std::map<std::pair<vertex_t, vertex_t>, std::map<vertex_t, std::set<vertex_t>>>;

class EGfdValidation : public GfdHandler {
public:
    std::vector<Gfd> GenerateSatisfiedGfds(graph_t const& graph, std::vector<Gfd> const& gfds);

    EGfdValidation() : GfdHandler() {};

    EGfdValidation(graph_t graph_, std::vector<Gfd> gfds_) : GfdHandler(graph_, gfds_) {}
};

}  // namespace algos
