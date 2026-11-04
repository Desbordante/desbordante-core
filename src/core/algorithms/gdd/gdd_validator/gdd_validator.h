#pragma once

#include <filesystem>
#include <memory>
#include <span>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/config/thread_number/type.h"

namespace algos {

class GddValidator : public Algorithm, protected model::gdd::GraphTypes {
private:
    std::filesystem::path graph_path_;
    Graph graph_;
    std::vector<model::Gdd> gdds_;
    std::vector<model::Gdd> result_;
    std::vector<std::size_t> matches_count_;
    config::ThreadNumType threads_ = 1;

    struct PatternGrouping;
    struct ValidationExecutor;
    struct SequentialValidationExecutor;
    struct ParallelValidationExecutor;

protected:
    using GddCounterexample = model::GddCounterexample;

private:
    std::vector<GddCounterexample> counterexamples_;

    void RegisterOptions();

    virtual void ExecuteInternal() final;

    virtual void ResetState() final;

    virtual void LoadDataInternal() final;

    virtual void MakeExecuteOptsAvailable() final;

protected:
    struct GddHoldsResult {
        std::optional<GddCounterexample> ce;
        std::size_t match_count = 0;
    };

    // GraphMatcher callback: checks every GDD of the group on a match
    // returns false when every GDD of the group has a counterexample
    struct GroupCheck {
        std::span<model::Gdd const* const> group;
        Graph const* graph = nullptr;
        std::span<GddHoldsResult> output;
        std::size_t remaining = 0;

        bool operator()(Mapping const& match);
    };

    // every GDD in a group shares the same pattern, so we want to match it once if it
    // is possible
    virtual void HoldsGroup(std::span<model::Gdd const* const> group, Graph const& graph,
                            std::span<GddHoldsResult> output) = 0;

    virtual std::unique_ptr<GddValidator> CreateWorker() const = 0;

    virtual void ResetWorkerState() {}

public:
    GddValidator();

    std::vector<model::Gdd> const& GetResult() const noexcept {
        return result_;
    }

    std::vector<GddCounterexample> const& GetCounterexamples() const noexcept {
        return counterexamples_;
    }

    std::vector<std::size_t> const& GetMatchesCount() const {
        return matches_count_;
    }

    GddValidator(GddValidator const&) = delete;
    GddValidator(GddValidator&&) = delete;
    GddValidator& operator=(GddValidator const&) = delete;
    GddValidator& operator=(GddValidator&&) = delete;
    virtual ~GddValidator() override = default;
};

}  // namespace algos
