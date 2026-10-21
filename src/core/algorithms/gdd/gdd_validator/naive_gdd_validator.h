#pragma once

#include <span>

#include "gdd_validator.h"

namespace algos {

class NaiveGddValidator : public GddValidator {
private:
    DomainT domain_;

    // returns true when a counterexample is found for every GDD of the group
    bool FindCounterexamples(std::span<model::Gdd const* const> group,
                             model::gdd::graph_t const& graph, model::gdd::graph_t const& pattern,
                             MappingT& partial_map, std::span<GddHoldsResult> output,
                             std::size_t& remaining);

    static bool GraphHasCompatibleEdge(model::gdd::graph_t const& graph, VertexT graph_src,
                                       VertexT graph_dst, std::string const& pattern_edge_label);
    static bool AllPatternEdgesArePreserved(model::gdd::graph_t const& graph,
                                            model::gdd::graph_t const& pattern, VertexT pattern_src,
                                            VertexT pattern_dst, VertexT graph_src,
                                            VertexT graph_dst);
    static bool CanExtendMapping(model::gdd::graph_t const& graph, MappingT const& partial_map,
                                 model::gdd::graph_t const& pattern, VertexT pattern_var,
                                 VertexT graph_vertex);

protected:
    virtual void HoldsGroup(std::span<model::Gdd const* const> group,
                            model::gdd::graph_t const& graph,
                            std::span<GddHoldsResult> output) final;

    virtual std::unique_ptr<GddValidator> CreateWorker() const final;

public:
    NaiveGddValidator() = default;
};

}  // namespace algos
