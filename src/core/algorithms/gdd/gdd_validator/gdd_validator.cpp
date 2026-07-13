#include "core/algorithms/gdd/gdd_validator/gdd_validator.h"

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/parser/graph_parser/graph_parser.h"

namespace algos {

void GddValidator::ResetState() {
    result_.clear();
    counterexamples_.clear();
    matches_count_.clear();
    matches_count_.reserve(gdds_.size());
}

void GddValidator::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option{&graph_path_, kGraphData, kDGraphData});
    RegisterOption(Option{&gdds_, kGddData, kDGddData});
    MakeOptionsAvailable({kGraphData, kGddData});
}

GddValidator::GddValidator() : Algorithm() {
    RegisterOptions();
}

void GddValidator::LoadDataInternal() {
    graph_ = parser::graph_parser::gdd::ReadGraph(graph_path_);
}

void GddValidator::ExecuteInternal() {
    ResetState();

    std::size_t gdd_index = 0;
    std::ranges::copy_if(gdds_, std::back_inserter(result_),
                         [this, &gdd_index](model::Gdd const& gdd) {
                             auto [ce, match_count] = Holds(gdd, graph_);
                             matches_count_.push_back(match_count);
                             if (ce.has_value()) {
                                 ce->gdd_index = gdd_index;
                                 counterexamples_.emplace_back(std::move(*ce));
                                 ++gdd_index;
                                 return false;
                             }
                             ++gdd_index;
                             return true;
                         });
}

GddValidator::DomainT GddValidator::BuildDomain(model::gdd::graph_t const& pattern,
                                                model::gdd::graph_t const& graph) {
    DomainT dom;

    for (auto [pv, pend] = boost::vertices(pattern); pv != pend; ++pv) {
        for (auto [gv, gend] = boost::vertices(graph); gv != gend; ++gv) {
            if (LabelsMatch(pattern[*pv].label, graph[*gv].label)) {
                dom[*pv].emplace_back(*gv);
            }
        }
    }

    return dom;
}

}  // namespace algos
