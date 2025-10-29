#pragma once
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/gfd/gfd.h"
#include "algorithms/gfd/gfd_validator/gfd_handler.h"
#include "gfd/graph_descriptor.h"

namespace algos {

class NaiveGfdValidator : public GfdHandler {
public:
    std::vector<model::Gfd> GenerateSatisfiedGfds(model::graph_t const& graph,
                                                  std::vector<model::Gfd> const& gfds);

    NaiveGfdValidator() : GfdHandler() {};

    NaiveGfdValidator(model::graph_t graph_, std::vector<model::Gfd> gfds_)
        : GfdHandler(graph_, gfds_) {}
};

}  // namespace algos
