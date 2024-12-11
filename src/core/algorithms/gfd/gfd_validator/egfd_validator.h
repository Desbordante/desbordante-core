#pragma once
#include "algorithms/gfd/gfd.h"
#include "algorithms/gfd/gfd_validator/gfd_handler.h"
#include "config/names_and_descriptions.h"

namespace algos {

namespace egfd_validator {

using CPI = std::map<std::pair<model::vertex_t, model::vertex_t>,
                     std::map<model::vertex_t, std::set<model::vertex_t>>>;

}  // namespace egfd_validator

class EGfdValidator : public GfdHandler {
public:
    std::vector<model::Gfd> GenerateSatisfiedGfds(model::graph_t const& graph,
                                                  std::vector<model::Gfd> const& gfds);

    EGfdValidator() : GfdHandler() {};

    EGfdValidator(model::graph_t graph_, std::vector<model::Gfd> gfds_)
        : GfdHandler(graph_, gfds_) {}
};

}  // namespace algos
