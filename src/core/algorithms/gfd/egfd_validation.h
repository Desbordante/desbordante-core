#pragma once
#include "algorithms/gfd/gfd_handler.h"
#include "config/names_and_descriptions.h"
#include "gfd.h"

namespace algos {

using CPI = std::map<std::pair<vertex_t, vertex_t>, std::map<vertex_t, std::set<vertex_t>>>;

class EGfdValidation : public GfdHandler {
public:
    std::vector<Gfd> GenerateSatisfiedGfds(graph_t const& graph, std::vector<Gfd> const& gfds);

    EGfdValidation() : GfdHandler(){};

    EGfdValidation(graph_t graph_, std::vector<Gfd> gfds_) : GfdHandler(graph_, gfds_) {}
};

}  // namespace algos
