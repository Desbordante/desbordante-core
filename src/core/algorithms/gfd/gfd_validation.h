#pragma once
#include <thread>

#include "algorithms/algorithm.h"
#include "algorithms/gfd/gfd_handler.h"
#include "config/names_and_descriptions.h"
#include "config/thread_number/type.h"
#include "gfd.h"

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
