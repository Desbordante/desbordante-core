#include "core/algorithms/gdd/gdd_validator/gdd_validator.h"

#include <chrono>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/util/timed_invoke.h"

namespace algos {

unsigned long long GddValidator::ExecuteInternal() {
    return util::TimedInvoke(&GddValidator::FilterValidGdds, this);
}

GddValidator::GddValidator() : Algorithm({}) {}

GddValidator::GddValidator(model::gdd::graph_t const& graph, std::vector<model::Gdd> gdds)
    : Algorithm({}), graph_(graph), gdds_(std::move(gdds)) {}

void GddValidator::FilterValidGdds() {
    result_.clear();
    std::ranges::copy_if(gdds_, std::back_inserter(result_),
                         [this](model::Gdd const& gdd) { return Holds(gdd, graph_); });

}

}  // namespace algos
