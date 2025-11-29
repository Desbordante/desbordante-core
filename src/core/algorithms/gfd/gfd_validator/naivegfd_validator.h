#pragma once
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gfd/gfd.h"
#include "core/algorithms/gfd/gfd_validator/gfd_handler.h"

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
