#pragma once

#include "gdd_validator.h"

namespace algos {

class NaiveGddValidator : public GddValidator {
private:
    using VertexT = model::gdd::vertex_t;
    using EdgeT = model::gdd::edge_t;
    using DomainT = std::unordered_map<VertexT, std::vector<VertexT>>;
    using MappingT = std::unordered_map<VertexT, VertexT>;

    DomainT domain_;

    static DomainT BuildDomain(model::gdd::graph_t const& pattern,
                               model::gdd::graph_t const& graph);
    static GddCounterexample BuildCounterexample(model::gdd::graph_t const& pattern,
                                                 model::gdd::graph_t const& graph,
                                                 MappingT const& mapping);
    bool ExistsCounterexample(model::Gdd const& gdd, model::gdd::graph_t const& graph,
                              MappingT& partial_map, GddCounterexample& counterexample);

protected:
    virtual bool Holds(model::Gdd const& gdd, model::gdd::graph_t const& graph,
                       GddCounterexample& out_counterexample) final;

public:
    NaiveGddValidator() = default;
};

}  // namespace algos
