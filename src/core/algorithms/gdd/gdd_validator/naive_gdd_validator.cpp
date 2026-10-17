#include "naive_gdd_validator.h"

#include <algorithm>
#include <cassert>
#include <span>

#include "core/algorithms/gdd/graph_matcher/backtracking_graph_matcher.h"

namespace algos {

void NaiveGddValidator::HoldsGroup(std::span<model::Gdd const* const> group, Graph const& graph,
                                   std::span<GddHoldsResult> output) {
    assert(group.size() == output.size());
    std::ranges::fill(output, GddHoldsResult{});

    if (group.empty()) {
        return;
    }

    BacktrackingGraphMatcher<GroupCheck> matcher(
            graph,
            GroupCheck{
                    .group = group, .graph = &graph, .output = output, .remaining = group.size()});
    matcher.Run(group.front()->GetPattern());
}

std::unique_ptr<GddValidator> NaiveGddValidator::CreateWorker() const {
    return std::make_unique<NaiveGddValidator>();
}

}  // namespace algos
