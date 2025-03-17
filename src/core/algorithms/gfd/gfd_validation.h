#pragma once

#include <tuple>   // for tuple
#include <vector>  // for vector

#include <boost/iterator/iterator_facade.hpp>  // for operator!=

#include "algorithms/gfd/gfd_handler.h"  // for GfdHandler
#include "config/thread_number/type.h"   // for ThreadNumType
#include "gfd.h"                         // for Gfd
#include "gfd/graph_descriptor.h"        // for vertex_t, graph_t

namespace algos {

using Request = std::tuple<int, vertex_t, int, std::vector<vertex_t>>;
using Message = std::tuple<int, vertex_t, vertex_t>;

class GfdValidation : public GfdHandler {
private:
    config::ThreadNumType threads_num_;

public:
    std::vector<Gfd> GenerateSatisfiedGfds(graph_t const& graph, std::vector<Gfd> const& gfds);

    GfdValidation();

    GfdValidation(graph_t graph_, std::vector<Gfd> gfds_) : GfdHandler(graph_, gfds_) {}
};

}  // namespace algos
