#pragma once

#include <span>

#include "gdd_validator.h"

namespace algos {

class NaiveGddValidator : public GddValidator {
protected:
    virtual void HoldsGroup(std::span<model::Gdd const* const> group, Graph const& graph,
                            std::span<GddHoldsResult> output) final;

    virtual std::unique_ptr<GddValidator> CreateWorker() const final;

public:
    NaiveGddValidator() = default;
};

}  // namespace algos
