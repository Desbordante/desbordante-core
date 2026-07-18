#pragma once

#include "gdd_validator.h"

namespace algos {

class NaiveGddValidator : public GddValidator {
private:
    DomainT domain_;

    bool ExistsCounterexample(model::Gdd const& gdd, model::gdd::graph_t const& graph,
                              MappingT& partial_map, GddCounterexample& counterexample);

    static bool GraphHasCompatibleEdge(model::gdd::graph_t const& graph, VertexT graph_src,
                                       VertexT graph_dst, std::string const& pattern_edge_label);
    static bool AllPatternEdgesArePreserved(model::gdd::graph_t const& graph,
                                            model::gdd::graph_t const& pattern, VertexT pattern_src,
                                            VertexT pattern_dst, VertexT graph_src,
                                            VertexT graph_dst);
    static bool CanExtendMapping(model::gdd::graph_t const& graph, MappingT const& partial_map,
                                 model::gdd::graph_t const& pattern, VertexT pattern_var,
                                 VertexT graph_vertex);

    std::size_t match_count_ = 0;

protected:
    virtual GddHoldsResult Holds(model::Gdd const& gdd, model::gdd::graph_t const& graph) final;

    virtual std::unique_ptr<GddValidator> CreateWorker() const final;

public:
    NaiveGddValidator() = default;
};

}  // namespace algos
