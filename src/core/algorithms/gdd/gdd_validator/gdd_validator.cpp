#include "core/algorithms/gdd/gdd_validator/gdd_validator.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/gdd/gdd.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/thread_number/option.h"
#include "core/parser/graph_parser/graph_parser.h"
#include "gdd_pattern_grouping.h"
#include "gdd_validation_executor.h"

namespace algos {

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
    PatternGrouping const grouping = PatternGrouping::GroupByPattern(gdds_);
    std::size_t const group_count = grouping.GroupCount();

    std::vector<GddHoldsResult> grouped_results(gdds_.size());

    std::unique_ptr<ValidationExecutor> executor = std::make_unique<SequentialValidationExecutor>();
    if (std::size_t const actual_threads = std::min<std::size_t>(threads_, group_count);
        actual_threads > 1) {
        executor =
                std::make_unique<ParallelValidationExecutor>(std::move(executor), actual_threads);
    }
    executor->Execute(*this, grouping, 0, group_count, graph_, grouped_results);

    // undo the reordering done by the grouping: results are reported in input order
    std::vector<GddHoldsResult> ordered_results(gdds_.size());
    for (std::size_t i = 0; i < grouping.origin.size(); ++i) {
        ordered_results[grouping.origin[i]] = std::move(grouped_results[i]);
    }

    result_.reserve(gdds_.size());
    for (std::size_t gdd_index = 0; gdd_index < gdds_.size(); ++gdd_index) {
        auto& [ce, match_count] = ordered_results[gdd_index];
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
