#pragma once

#include <cstddef>
#include <span>

#include "core/util/worker_thread_pool.h"
#include "gdd_validator.h"

namespace algos {

struct GddValidator::ValidationExecutor {
    virtual void Execute(GddValidator& validator, PatternGrouping const& grouping,
                         std::size_t first_group, std::size_t last_group,
                         model::gdd::graph_t const& graph,
                         std::span<GddHoldsResult> output) const = 0;

    virtual ~ValidationExecutor() = default;
};

struct GddValidator::SequentialValidationExecutor : ValidationExecutor {
    virtual void Execute(GddValidator& validator, PatternGrouping const& grouping,
                         std::size_t first_group, std::size_t last_group,
                         model::gdd::graph_t const& graph,
                         std::span<GddHoldsResult> output) const final;
};

struct GddValidator::ParallelValidationExecutor : ValidationExecutor {
private:
    std::unique_ptr<ValidationExecutor> inner_;
    std::size_t threads_;

public:
    ParallelValidationExecutor(std::unique_ptr<ValidationExecutor> inner, std::size_t threads)
        : inner_(std::move(inner)), threads_(threads) {
        assert(inner_ != nullptr);
        assert(threads_ > 1);
    }

    virtual void Execute(GddValidator& validator, PatternGrouping const& grouping,
                         std::size_t first_group, std::size_t last_group,
                         model::gdd::graph_t const& graph,
                         std::span<GddHoldsResult> output) const final;
};

}  // namespace algos
