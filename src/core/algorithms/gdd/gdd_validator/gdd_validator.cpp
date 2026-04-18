#include "core/algorithms/gdd/gdd_validator/gdd_validator.h"

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/parser/graph_parser/graph_parser.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"

namespace algos {

unsigned long long GddValidator::ExecuteInternal() {
    return util::TimedInvoke(&GddValidator::FilterValidGdds, this);
}

void GddValidator::ResetState() {
    result_.clear();
    counterexamples_.clear();
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
    LOG_ERROR("{}", graph_path_.string());
    graph_ = parser::graph_parser::gdd::ReadGraph(graph_path_);
}

void GddValidator::FilterValidGdds() {
    ResetState();

    std::size_t gdd_index = 0;
    std::ranges::copy_if(gdds_, std::back_inserter(result_),
                         [this, &gdd_index](model::Gdd const& gdd) {
                             if (GddCounterexample ce{}; !Holds(gdd, graph_, ce)) {
                                 ce.gdd_index = gdd_index;
                                 counterexamples_.emplace_back(std::move(ce));
                                 ++gdd_index;
                                 return false;
                             }
                             ++gdd_index;
                             return true;
                         });
}

}  // namespace algos
