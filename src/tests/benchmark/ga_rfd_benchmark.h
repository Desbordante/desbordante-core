#pragma once

#include <iostream>

#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/config/names.h"
#include "tests/benchmark/benchmark_comparer.h"
#include "tests/benchmark/benchmark_runner.h"
#include "tests/common/all_csv_configs.h"

namespace benchmark {

inline void GaRfdBenchmark(BenchmarkRunner& runner, [[maybe_unused]] BenchmarkComparer& comparer) {
    using namespace config::names;

    auto test = [] {
        config::InputTable table = std::make_shared<CSVParser>(tests::kNeighbors10k);

        auto eq = algos::rfd::EqualityMetric();
        auto lev = algos::rfd::LevenshteinMetric();

        std::vector<std::shared_ptr<algos::rfd::SimilarityMetric>> metrics{lev, lev, lev, eq,
                                                                           eq,  eq,  eq};

        algos::StdParamsMap params{{kTable, table},
                                   {kRfdMinSimilarity, 0.9},
                                   {kMinimumConfidence, 0.9},
                                   {kPopulationSize, std::size_t{924}},
                                   {kRfdMaxGenerations, std::size_t{13}},
                                   {kRfdCrossoverProbability, 0.85},
                                   {kRfdMutationProbability, 0.3},
                                   {kSeed, std::uint64_t{123}},
                                   {"metrics", metrics}};

        auto algo = algos::CreateAndLoadAlgorithm<algos::rfd::GaRfd>(params);
        algo->Execute();

        auto rfds = algo->GetRfds();
        std::cout << "Found " << rfds.size() << " RFD(s):\n";
        for (const auto& rfd : rfds) {
            std::cout << "  " << rfd.ToString() << '\n';
        }
    };

    runner.RegisterBenchmark("GaRfd, neighbors10k (explicit params)", std::move(test));
}

}  // namespace benchmark
