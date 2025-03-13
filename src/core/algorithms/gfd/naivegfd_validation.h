#pragma once

#include <vector>  // for vector

#include <boost/iterator/iterator_facade.hpp>  // for operator!=

#include "algorithms/gfd/gfd_handler.h"  // for GfdHandler
#include "gfd.h"                         // for Gfd
#include "gfd/graph_descriptor.h"        // for graph_t

namespace algos {

class NaiveGfdValidation : public GfdHandler {
public:
    std::vector<Gfd> GenerateSatisfiedGfds(graph_t const& graph, std::vector<Gfd> const& gfds);

    NaiveGfdValidation() : GfdHandler() {};

    NaiveGfdValidation(graph_t graph_, std::vector<Gfd> gfds_) : GfdHandler(graph_, gfds_) {}
};

}  // namespace algos
