#include "core/algorithms/fd/fdhits/fdhits.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/fdhits/edgecollectors/edge_hash_collector.h"
#include "core/algorithms/fd/fdhits/edgecollectors/vec_edge_collector.h"
#include "core/algorithms/fd/fdhits/edges/default_edge.h"
#include "core/algorithms/fd/fdhits/edges/multi_fd.h"
#include "core/algorithms/fd/fdhits/pli_table.h"
#include "core/algorithms/fd/fdhits/problems/fd_problem.h"
#include "core/algorithms/fd/fdhits/problems/multi_fd_problem.h"
#include "core/algorithms/fd/fdhits/sampling/fd_sampler.h"
#include "core/algorithms/fd/fdhits/treesearch/treesearch_de.h"
#include "core/algorithms/fd/fdhits/treesearch/treesearch_multi.h"
#include "core/algorithms/fd/fdhits/validator/pli_validator.h"
#include "core/algorithms/fd/hycommon/preprocessor.h"
#include "core/algorithms/fd/hycommon/util/pli_util.h"
#include "core/config/fdhits_mode/option.h"
#include "core/config/names.h"
#include "core/util/logger.h"

namespace algos::fd::fdhits {

FDHits::FDHits() : PliBasedFDAlgorithm({}) {
    RegisterOption(config::kFDHitsModeOpt(&mode_));
}

void FDHits::MakeExecuteOptsAvailableFDInternal() {
    MakeOptionsAvailable({config::names::kFDHitsMode});
}

void FDHits::RegisterFDs(std::vector<std::pair<std::vector<size_t>, size_t>> const& fds,
                         std::vector<hy::ClusterId> const& og_mapping) {
    auto const* schema = GetRelation().GetSchema();
    for (auto const& [lhs_indices, rhs_index] : fds) {
        boost::dynamic_bitset<> mapped_lhs(schema->GetNumColumns());
        for (size_t idx : lhs_indices) {
            auto mapped = og_mapping[idx];
            mapped_lhs.set(mapped);
        }
        Vertical lhs_v(schema, std::move(mapped_lhs));

        auto const mapped_rhs = og_mapping[rhs_index];
        Column rhs_c(schema, schema->GetColumn(mapped_rhs)->GetName(), mapped_rhs);

        RegisterFd(std::move(lhs_v), std::move(rhs_c), relation_->GetSharedPtrSchema());
    }
}

unsigned long long FDHits::ExecuteInternal() {
    auto const start_time = std::chrono::system_clock::now();

    auto [plis, pli_records, og_mapping] = algos::hy::Preprocess(relation_.get());
    auto inverse = algos::hy::util::BuildInvertedPlis(plis);

    auto plis_shared = std::make_shared<hy::PLIs>(std::move(plis));
    auto inverse_shared = std::make_shared<hy::Columns>(std::move(inverse));
    auto records_shared = std::make_shared<hy::Rows>(std::move(pli_records));

    PLITable pli_table(plis_shared, inverse_shared, records_shared,
                       relation_->GetSchema()->GetName());

    size_t const column_count = pli_table.GetColumnCount();
    LOG_DEBUG("FDHits started: mode={}, columns={}", mode_, column_count);

    std::vector<std::pair<std::vector<size_t>, size_t>> raw_fds;

    if (mode_ == "per_target") {
        struct TargetFDCollector {
            size_t target{};
            std::vector<std::pair<std::vector<size_t>, size_t>>* sink{};

            void Consume(edges::DefaultEdge const& lhs) {
                sink->emplace_back(lhs.GetNodes(), target);
            }
        };

        for (size_t target = 0; target < pli_table.GetColumnCount(); ++target) {
            using Problem = problems::FDProblem;
            using EdgeType = edges::DefaultEdge;

            Problem problem(target, &pli_table);
            Sampler<Problem> sampling(&problem, &pli_table, sampling_factor_);

            EdgeHashCollector<EdgeType> collector;
            for (size_t c = 0; c < pli_table.GetColumnCount(); ++c) {
                if (c != target) {
                    sampling.SamplePLI(pli_table.GetColumnPli(c), &collector);
                }
            }

            Hypergraph<EdgeType> input =
                    collector.ToGraph(EdgeType::Filled(pli_table.GetColumnCount()));

            TargetFDCollector consumer{target, &raw_fds};

            PLIValidator<EdgeType, TargetFDCollector, Sampler<Problem>, Problem> validator(
                    &pli_table, &consumer, &problem, &sampling);

            SearchDE(&input, &validator);
        }
    } else {
        using Problem = problems::MultiFDProblem;
        using EdgeType = Problem::EdgeType;
        Problem problem(&pli_table, false);

        Sampler<Problem> sampling(&problem, &pli_table, sampling_factor_);

        EdgeHashCollector<EdgeType> collector;
        for (size_t c = 0; c < pli_table.GetColumnCount(); ++c) {
            sampling.SamplePLI(pli_table.GetColumnPli(c), &collector);
        }

        Hypergraph<EdgeType> input =
                collector.ToGraph(EdgeType::Filled(pli_table.GetColumnCount() * 2));

        VecEdgeCollector<EdgeType> results_collector;

        PLIValidator<EdgeType, VecEdgeCollector<EdgeType>, Sampler<Problem>, Problem> validator(
                &pli_table, &results_collector, &problem, &sampling);

        SearchMulti(&input, &validator);

        for (auto const& fd : results_collector.results) {
            auto lhs_nodes = fd.GetLhs().GetNodes();
            auto rhs_nodes = fd.GetRhs().GetNodes();
            for (size_t rhs : rhs_nodes) {
                raw_fds.emplace_back(lhs_nodes, rhs);
            }
        }
    }

    std::sort(raw_fds.begin(), raw_fds.end(), [](auto const& a, auto const& b) {
        if (a.second != b.second) return a.second < b.second;
        return a.first < b.first;
    });
    raw_fds.erase(std::unique(raw_fds.begin(), raw_fds.end()), raw_fds.end());

    RegisterFDs(raw_fds, og_mapping);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);

    LOG_DEBUG("FDHits completed: {} FDs found in {} ms", raw_fds.size(), elapsed.count());

    return elapsed.count();
}

}  // namespace algos::fd::fdhits
