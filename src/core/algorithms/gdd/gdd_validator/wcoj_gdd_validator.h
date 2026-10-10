#pragma once

#include <functional>
#include <optional>
#include <span>

#include "core/algorithms/gdd/graph_matcher/wcoj_graph_matcher.h"
#include "gdd_validator.h"

namespace algos {

class WcojGddValidator : public GddValidator {
private:
    // the matcher outlives a group (its adjacency index is reused), while the check state is
    // per group: the matcher calls check_ by reference, and HoldsGroup refills it
    GroupCheck check_;
    std::optional<WcojGraphMatcher<std::reference_wrapper<GroupCheck>>> matcher_;

protected:
    virtual void HoldsGroup(std::span<model::Gdd const* const> group, Graph const& graph,
                            std::span<GddHoldsResult> output) final;

    virtual std::unique_ptr<GddValidator> CreateWorker() const final;
    virtual void ResetWorkerState() final;

public:
    WcojGddValidator() = default;
};

}  // namespace algos
