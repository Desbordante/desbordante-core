#pragma once

#include <filesystem>
#include <memory>
#include <span>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/algorithms/gdd/gdd_graph_description.h"
#include "core/config/thread_number/type.h"

namespace algos {

class GddValidator : public Algorithm {
private:
    std::filesystem::path graph_path_;
    model::gdd::graph_t graph_;
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
    using VertexT = model::gdd::detail::VertexT;
    using EdgeT = model::gdd::detail::EdgeT;
    using DomainT = model::gdd::detail::DomainT;
    using MappingT = model::gdd::detail::MappingT;

    static DomainT BuildDomain(model::gdd::graph_t const& pattern,
                               model::gdd::graph_t const& graph);

    static bool LabelsMatch(std::string const& lhs, std::string const& rhs) noexcept {
        return lhs == rhs;  // TODO: wildcards
    }

    struct GddHoldsResult {
        std::optional<GddCounterexample> ce;
        std::size_t match_count = 0;
    };

    // every GDD in a group shares the same pattern, so we want to match it once if it
    // is possible
    virtual void HoldsGroup(std::span<model::Gdd const* const> group,
                            model::gdd::graph_t const& graph, std::span<GddHoldsResult> output) = 0;

    virtual std::unique_ptr<GddValidator> CreateWorker() const = 0;

public:
    GddValidator();

    std::vector<model::Gdd> const& GetResult() const noexcept {
        return result_;
    }

    std::vector<GddCounterexample> const& GetCounterexamples() const noexcept {
        return counterexamples_;
    }

    std::vector<std::size_t> GetMatchesCount() const {
        return matches_count_;
    }

    GddValidator(GddValidator const&) = delete;
    GddValidator(GddValidator&&) = delete;
    GddValidator& operator=(GddValidator const&) = delete;
    GddValidator& operator=(GddValidator&&) = delete;
    virtual ~GddValidator() override = default;
};

}  // namespace algos
