#include "gdd_validation_executor.h"

#include "gdd_pattern_grouping.h"

namespace algos {

void GddValidator::SequentialValidationExecutor::Execute(GddValidator& validator,
                                                         PatternGrouping const& grouping,
                                                         std::size_t first_group,
                                                         std::size_t last_group,
                                                         model::gdd::graph_t const& graph,
                                                         std::span<GddHoldsResult> output) const {
    assert(last_group <= grouping.GroupCount());
    assert(grouping.gdds.size() == output.size());

    for (std::size_t group = first_group; group < last_group; ++group) {
        std::size_t const begin = grouping.starts[group];
        std::size_t const size = grouping.starts[group + 1] - begin;

        validator.HoldsGroup(std::span{grouping.gdds}.subspan(begin, size), graph,
                             output.subspan(begin, size));
    }
}

void GddValidator::ParallelValidationExecutor::Execute(GddValidator& validator,
                                                       PatternGrouping const& grouping,
                                                       std::size_t first_group,
                                                       std::size_t last_group,
                                                       model::gdd::graph_t const& graph,
                                                       std::span<GddHoldsResult> output) const {
    assert(last_group <= grouping.GroupCount());
    assert(last_group - first_group >= threads_);

    util::WorkerThreadPool pool{threads_};
    pool.ExecIndexWithResource(
            [this, &grouping, first_group, &graph, output](
                    model::Index index, std::unique_ptr<GddValidator> const& worker) {
                std::size_t const group = first_group + index;
                inner_->Execute(*worker, grouping, group, group + 1, graph, output);
            },
            [&validator] { return validator.CreateWorker(); }, last_group - first_group);
}

}  // namespace algos
