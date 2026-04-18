#pragma once

#include "gdd_validator.h"

namespace algos {

class NaiveGddValidator : public GddValidator {
private:
    using vertex_t = model::gdd::vertex_t;
    using edge_t = model::gdd::edge_t;
    using DomainT = std::unordered_map<vertex_t, std::vector<vertex_t>>;
    using MappingT = std::unordered_map<vertex_t, vertex_t>;

    DomainT domain_;

    static DomainT BuildDomain(model::gdd::graph_t const& pattern,
                               model::gdd::graph_t const& graph);
    static GddCounterexample BuildCounterexample(model::gdd::graph_t const& pattern,
                                                 model::gdd::graph_t const& graph,
                                                 MappingT const& mapping);
    bool ExistsCounterexample(model::Gdd const& gdd, model::gdd::graph_t const& graph,
                              MappingT& partial_map, GddCounterexample* counterexample);

protected:
    virtual bool Holds(model::Gdd const& gdd, model::gdd::graph_t const& graph,
                       GddCounterexample* out_counterexample = nullptr) final;

public:
    NaiveGddValidator() = default;

    NaiveGddValidator(model::gdd::graph_t const& graph, std::vector<model::Gdd> gdds)
        : GddValidator(graph, std::move(gdds)) {}
};

}  // namespace algos
