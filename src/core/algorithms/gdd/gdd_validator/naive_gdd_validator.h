#pragma once

#include "gdd_validator.h"

namespace algos {

class NaiveGddValidator : public GddValidator {
private:
    using VertexT = model::gdd::vertex_t;
    using EdgeT = model::gdd::edge_t;
    using DomainT = std::unordered_map<VertexT, std::vector<VertexT>>;
    using MappingT = std::unordered_map<VertexT, VertexT>;
    using GddCounterexample = model::GddCounterexample;

    DomainT domain_;

    static DomainT BuildDomain(model::gdd::graph_t const& pattern,
                               model::gdd::graph_t const& graph);
    bool ExistsCounterexample(model::Gdd const& gdd, model::gdd::graph_t const& graph,
                              MappingT& partial_map, GddCounterexample& counterexample);

    bool GraphHasCompatibleEdge(VertexT graph_src, VertexT graph_dst,
                                std::string const& pattern_edge_label) const;
    bool AllPatternEdgesArePreserved(model::gdd::graph_t const& pattern, VertexT pattern_src,
                                     VertexT pattern_dst, VertexT graph_src,
                                     VertexT graph_dst) const;
    bool CanExtendMapping(MappingT const& partial_map, model::gdd::graph_t const& pattern,
                          VertexT pattern_var, VertexT graph_vertex) const;

protected:
    virtual std::optional<GddCounterexample> Holds(model::Gdd const& gdd,
                                                   model::gdd::graph_t const& graph) final;

public:
    NaiveGddValidator() = default;
};

}  // namespace algos
