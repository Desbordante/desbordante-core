#pragma once
#include <vector>

#include "algorithms/algorithm.h"
#include "algorithms/gfd/gfd.h"
#include "config/names_and_descriptions.h"
#include "parser/graph_parser/graph_parser.h"

namespace algos {

class GfdHandler : public Algorithm {
protected:
    std::filesystem::path graph_path_;
    std::vector<std::filesystem::path> gfd_paths_;

    model::graph_t graph_;
    std::vector<model::Gfd> gfds_;
    std::vector<model::Gfd> result_;

    unsigned long long ExecuteInternal();

    void ResetState() final;
    void LoadDataInternal() final;

    void RegisterOptions();

public:
    virtual std::vector<model::Gfd> GenerateSatisfiedGfds(model::graph_t const& graph,
                                                          std::vector<model::Gfd> const& gfds) = 0;

    GfdHandler();

    GfdHandler(model::graph_t graph_, std::vector<model::Gfd> gfds_)
        : Algorithm({}), graph_(graph_), gfds_(gfds_) {
        ExecutePrepare();
    }

    std::vector<model::Gfd> GfdList() {
        return result_;
    }
};

}  // namespace algos
