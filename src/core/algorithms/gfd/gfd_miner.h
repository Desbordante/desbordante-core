#pragma once
#include "algorithms/algorithm.h"
#include "config/names_and_descriptions.h"
#include "gfd.h"
#include "parser/graph_parser/graph_parser.h"

namespace algos {

class GfdMiner : public Algorithm {
protected:
    std::filesystem::path graph_path_;

    graph_t graph_;
    std::size_t k_;
    std::size_t sigma_;

    std::vector<Gfd> gfds_;

    unsigned long long ExecuteInternal();

    void ResetState() final;
    void LoadDataInternal() final;

    void RegisterOptions();

public:
    std::vector<Gfd> MineGfds(graph_t const& graph, std::size_t const k, std::size_t const sigma);

    GfdMiner();

    GfdMiner(graph_t graph_) : Algorithm({}), graph_(graph_) {
        ExecutePrepare();
    }

    std::vector<Gfd> const& GfdList() {
        return gfds_;
    }
};

}  // namespace algos
