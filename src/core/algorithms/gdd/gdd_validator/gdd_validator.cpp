#include "core/algorithms/gdd/gdd_validator/gdd_validator.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <span>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/thread_number/option.h"
#include "core/parser/graph_parser/graph_parser.h"
#include "core/util/worker_thread_pool.h"

namespace algos {

struct GddValidator::ValidationExecutor {
    virtual void Execute(GddValidator& validator, std::span<model::Gdd const> gdds,
                         model::gdd::graph_t const& graph,
                         std::span<GddHoldsResult> output) const = 0;

    virtual ~ValidationExecutor() = default;
};

struct GddValidator::SequentialValidationExecutor : ValidationExecutor {
    virtual void Execute(GddValidator& validator, std::span<model::Gdd const> gdds,
                         model::gdd::graph_t const& graph,
                         std::span<GddHoldsResult> output) const final {
        assert(gdds.size() == output.size());
        for (std::size_t i = 0; i < gdds.size(); ++i) {
            output[i] = validator.Holds(gdds[i], graph);
        }
    }
};

// decorator on top of another executor
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

    virtual void Execute(GddValidator& validator, std::span<model::Gdd const> gdds,
                         model::gdd::graph_t const& graph,
                         std::span<GddHoldsResult> output) const final {
        assert(gdds.size() == output.size());
        assert(gdds.size() >= threads_);

        util::WorkerThreadPool pool{threads_};
        pool.ExecIndexWithResource(
                [this, gdds, &graph, output](model::Index index,
                                             std::unique_ptr<GddValidator> const& worker) {
                    inner_->Execute(*worker, gdds.subspan(index, 1), graph,
                                    output.subspan(index, 1));
                },
                [&validator]() { return validator.CreateWorker(); }, gdds.size());
    }
};

void GddValidator::ResetState() {
    result_.clear();
    counterexamples_.clear();
    matches_count_.clear();
    matches_count_.reserve(gdds_.size());
}

void GddValidator::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option{&graph_path_, kGraphData, kDGraphData});
    RegisterOption(Option{&gdds_, kGddData, kDGddData});
    RegisterOption(config::kThreadNumberOpt(&threads_));
    MakeOptionsAvailable({kGraphData, kGddData});
}

GddValidator::GddValidator() : Algorithm() {
    RegisterOptions();
}

void GddValidator::LoadDataInternal() {
    graph_ = parser::graph_parser::gdd::ReadGraph(graph_path_);
}

void GddValidator::MakeExecuteOptsAvailable() {
    MakeOptionsAvailable({config::names::kThreads});
}

void GddValidator::ExecuteInternal() {
    std::vector<GddHoldsResult> check_results(gdds_.size());

    std::unique_ptr<ValidationExecutor> executor = std::make_unique<SequentialValidationExecutor>();
    if (std::size_t const actual_threads = std::min<std::size_t>(threads_, gdds_.size());
        actual_threads > 1) {
        executor =
                std::make_unique<ParallelValidationExecutor>(std::move(executor), actual_threads);
    }
    executor->Execute(*this, gdds_, graph_, check_results);

    result_.reserve(gdds_.size());
    for (std::size_t gdd_index = 0; gdd_index < gdds_.size(); ++gdd_index) {
        auto& [ce, match_count] = check_results[gdd_index];
        matches_count_.push_back(match_count);
        if (ce.has_value()) {
            ce->gdd_index = gdd_index;
            counterexamples_.emplace_back(std::move(*ce));
        } else {
            result_.push_back(gdds_[gdd_index]);
        }
    }
}

GddValidator::DomainT GddValidator::BuildDomain(model::gdd::graph_t const& pattern,
                                                model::gdd::graph_t const& graph) {
    DomainT dom;

    for (auto [pv, pend] = boost::vertices(pattern); pv != pend; ++pv) {
        for (auto [gv, gend] = boost::vertices(graph); gv != gend; ++gv) {
            if (LabelsMatch(pattern[*pv].label, graph[*gv].label)) {
                dom[*pv].emplace_back(*gv);
            }
        }
    }

    return dom;
}

}  // namespace algos
