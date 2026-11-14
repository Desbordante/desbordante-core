#pragma once
#include "core/algorithms/gfd/gfd.h"
#include "core/algorithms/gfd/gfd_validator/gfd_handler.h"
#include "core/config/names_and_descriptions.h"

namespace algos {

namespace egfd_validator {

using CPI = std::map<std::pair<model::gfd::vertex_t, model::gfd::vertex_t>,
                     std::map<model::gfd::vertex_t, std::set<model::gfd::vertex_t>>>;

}  // namespace egfd_validator

class EGfdValidator : public GfdHandler {
public:
    std::vector<model::Gfd> GenerateSatisfiedGfds(model::gfd::graph_t const& graph,
                                                  std::vector<model::Gfd> const& gfds);

    EGfdValidator() : GfdHandler() {};

    EGfdValidator(model::gfd::graph_t graph_, std::vector<model::Gfd> gfds_)
        : GfdHandler(graph_, gfds_) {}
};

}  // namespace algos
