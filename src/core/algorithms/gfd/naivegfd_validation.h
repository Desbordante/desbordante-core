#pragma once
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/gfd/gfd_handler.h"
#include "gfd.h"

namespace algos {

class NaiveGfdValidation : public GfdHandler {
public:
    std::vector<Gfd> GenerateSatisfiedGfds(graph_t const& graph, std::vector<Gfd> const& gfds);

    NaiveGfdValidation() : GfdHandler(){};

    NaiveGfdValidation(graph_t graph_, std::vector<Gfd> gfds_) : GfdHandler(graph_, gfds_) {}
};

}  // namespace algos
