#include "core/algorithms/gdd/gdd_validator/gdd_validator.h"

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/parser/graph_parser/graph_parser.h"
#include "core/util/timed_invoke.h"

namespace algos {

unsigned long long GddValidator::ExecuteInternal() {
    return util::TimedInvoke(&GddValidator::FilterValidGdds, this);
}

void GddValidator::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option{&graph_path_, kGraphData, kDGraphData});
    RegisterOption(Option{&gdds_, kGddData, kDGddData});
}

GddValidator::GddValidator() : Algorithm() {
    RegisterOptions();
    MakeOptionsAvailable({config::names::kGraphData, config::names::kGddData});
}

GddValidator::GddValidator(model::gdd::graph_t const& graph, std::vector<model::Gdd> gdds)
    : Algorithm(), graph_(graph), gdds_(std::move(gdds)) {}

void GddValidator::LoadDataInternal() {
    graph_ = parser::graph_parser::gdd::ReadGraph(graph_path_);
}

void GddValidator::FilterValidGdds() {
    result_.clear();
    std::ranges::copy_if(gdds_, std::back_inserter(result_),
                         [this](model::Gdd const& gdd) { return Holds(gdd, graph_); });
}

}  // namespace algos
