#pragma once
#include <thread>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gfd/gfd.h"
#include "core/algorithms/gfd/gfd_validator/gfd_handler.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/thread_number/type.h"

namespace algos {

namespace gfd_validator {

using Request = std::tuple<int, model::vertex_t, int, std::vector<model::vertex_t>>;
using Message = std::tuple<int, model::vertex_t, model::vertex_t>;

}  // namespace gfd_validator

class GfdValidator : public GfdHandler {
private:
    config::ThreadNumType threads_num_;

public:
    std::vector<model::Gfd> GenerateSatisfiedGfds(model::graph_t const& graph,
                                                  std::vector<model::Gfd> const& gfds);

    GfdValidator();

    GfdValidator(model::graph_t graph_, std::vector<model::Gfd> gfds_)
        : GfdHandler(graph_, gfds_) {}
};

}  // namespace algos
