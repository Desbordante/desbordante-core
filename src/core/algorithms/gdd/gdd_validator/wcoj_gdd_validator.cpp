#include "wcoj_gdd_validator.h"

#include <algorithm>
#include <cassert>
#include <span>

namespace algos {

void WcojGddValidator::HoldsGroup(std::span<model::Gdd const* const> group, Graph const& graph,
                                  std::span<GddHoldsResult> output) {
    assert(group.size() == output.size());
    std::ranges::fill(output, GddHoldsResult{});

    if (group.empty()) {
        return;
    }

    check_ = GroupCheck{
            .group = group, .graph = &graph, .output = output, .remaining = group.size()};
    // matching runs once for the pattern shared by the group
    if (!matcher_.has_value()) {
        matcher_.emplace(graph, std::ref(check_));
    }
    matcher_->Run(group.front()->GetPattern());
}

std::unique_ptr<GddValidator> WcojGddValidator::CreateWorker() const {
    return std::make_unique<WcojGddValidator>();
}

// prevent reuse of the adjacency index between calls like:
// algo.load_data(graph=g, gdd=gdd1)
// algo.execute()
// algo.load_data(graph=g, gdd=gdd2)
// algo.execute()
void WcojGddValidator::ResetWorkerState() {
    if (matcher_.has_value()) {
        matcher_->Reset();
    }
}

}  // namespace algos
