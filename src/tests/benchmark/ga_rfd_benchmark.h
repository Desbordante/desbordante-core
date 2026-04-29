#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "core/config/names.h"
#include "tests/benchmark/benchmark_comparer.h"
#include "tests/benchmark/benchmark_runner.h"
#include "tests/common/all_csv_configs.h"

namespace benchmark {

inline void GaRfdBenchmark(BenchmarkRunner& runner, [[maybe_unused]] BenchmarkComparer& comparer) {
    using namespace config::names;

    auto test = [] {
        auto eq = algos::rfd::EqualityMetric();
        auto lev = algos::rfd::LevenshteinMetric();
        auto abs_diff = algos::rfd::AbsoluteDifferenceMetric();

        std::vector<std::shared_ptr<algos::rfd::SimilarityMetric>> metrics{
                abs_diff, abs_diff, abs_diff, eq, eq, eq, eq};

        algos::StdParamsMap params{{kCsvConfig, tests::kNeighbors10k},
                                   {kRfdMinSimilarity, 0.9},
                                   {kMinimumConfidence, 0.9},
                                   {kPopulationSize, std::size_t{924}},
                                   {kRfdMaxGenerations, std::size_t{64}},
                                   {kRfdCrossoverProbability, 0.85},
                                   {kRfdMutationProbability, 0.3},
                                   {kSeed, std::uint64_t{123}},
                                   {"metrics", metrics}};

        auto algo = algos::CreateAndLoadAlgorithm<algos::rfd::GaRfd>(params);
        algo->Execute();

        auto rfds = algo->GetRfds();
        std::cout << "Found " << rfds.size() << " RFD(s):\n";
        for (auto const& rfd : rfds) {
            std::cout << "  " << rfd.ToString() << '\n';
        }
    };

    runner.RegisterBenchmark("GaRfd, neighbors10k (explicit params)", std::move(test));
}

}  // namespace benchmark
