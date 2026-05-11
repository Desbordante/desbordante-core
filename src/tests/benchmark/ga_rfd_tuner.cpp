#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <sys/resource.h>
#include <vector>

#include "core/algorithms/algo_factory.h"
#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "core/config/names.h"
#include "tests/common/all_csv_configs.h"

namespace {
namespace rfd = algos::rfd;
using namespace config::names;

struct BenchmarkResult {
    std::string dataset;
    double min_similarity;
    double min_confidence;
    size_t population_size;
    size_t max_generations;
    double crossover_prob;
    double mutation_prob;
    std::string metric_type;
    long long time_ms;
    long memory_kb;
    size_t rfd_count;
};

long GetPeakRssKb() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

std::vector<std::shared_ptr<rfd::SimilarityMetric>> MakeMetrics(std::string const& type,
                                                                size_t num_attrs) {
    auto eq = rfd::EqualityMetric();
    auto lev = rfd::LevenshteinMetric();
    auto abs_diff = rfd::AbsoluteDifferenceMetric();

    std::vector<std::shared_ptr<rfd::SimilarityMetric>> metrics;
    if (type == "equality") {
        metrics.assign(num_attrs, eq);
    } else if (type == "levenshtein") {
        metrics.assign(num_attrs, lev);
    } else if (type == "abs_diff") {
        metrics.assign(num_attrs, abs_diff);
    } else if (type == "mixed") {
        for (size_t i = 0; i < num_attrs; ++i) {
            if (i < num_attrs / 2)
                metrics.push_back(abs_diff);
            else
                metrics.push_back(eq);
        }
    } else {
        metrics.assign(num_attrs, eq);
    }
    return metrics;
}

BenchmarkResult RunBenchmark(CSVConfig const& csv, double min_sim, double beta, size_t pop_size,
                             size_t gens, double crossover_prob, double mutation_prob,
                             std::string const& metric_type, int runs = 3) {
    auto tmp_parser = std::make_shared<CSVParser>(csv);
    size_t num_attrs = tmp_parser->GetNumberOfColumns();
    auto metrics = MakeMetrics(metric_type, num_attrs);

    std::vector<long long> times;
    std::vector<long> memories;
    std::vector<long> rfd_count_arr;

    for (int run = 0; run < runs; ++run) {
        algos::StdParamsMap params = {{kCsvConfig, csv},
                                      {kRfdMinSimilarity, min_sim},
                                      {kRfdMinimumConfidence, beta},
                                      {kPopulationSize, pop_size},
                                      {kRfdMaxGenerations, gens},
                                      {kRfdCrossoverProbability, crossover_prob},
                                      {kRfdMutationProbability, mutation_prob},
                                      {kSeed, static_cast<uint32_t>(42 + run)},
                                      {"metrics", metrics}};

        auto algo = algos::CreateAndLoadAlgorithm<rfd::GaRfd>(params);

        auto start = std::chrono::steady_clock::now();
        algo->Execute();
        auto end = std::chrono::steady_clock::now();
        long long time_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        long memory_kb = GetPeakRssKb();
        size_t rfd_count = algo->GetRfds().size();

        times.push_back(time_ms);
        memories.push_back(memory_kb);
        rfd_count_arr.push_back(rfd_count);
    }

    BenchmarkResult res;
    res.dataset = csv.path.stem().string();
    res.min_similarity = min_sim;
    res.min_confidence = beta;
    res.population_size = pop_size;
    res.max_generations = gens;
    res.crossover_prob = crossover_prob;
    res.mutation_prob = mutation_prob;
    res.metric_type = metric_type;
    res.time_ms = std::accumulate(times.begin(), times.end(), 0LL) / runs;
    res.memory_kb = std::accumulate(memories.begin(), memories.end(), 0L) / runs;
    res.rfd_count = std::accumulate(rfd_count_arr.begin(), rfd_count_arr.end(), 0L) / runs;

    return res;
}
}  // namespace

int main() {  // NOLINT
    std::vector<BenchmarkResult> results;

    std::vector<CSVConfig> datasets = {tests::kBreastCancer};

    std::vector<double> min_sims = {0.95};
    std::vector<double> betas = {0.95};
    std::vector<size_t> pop_sizes = {550};
    std::vector<size_t> generations = {32};
    std::vector<double> crossovers = {0.0, 0.2, 0.4, 0.6, 0.8, 1.0};
    std::vector<double> mutations = {0.0, 0.2, 0.4, 0.6, 0.8, 1.0};

    std::vector<std::string> metric_types = {"equality", "abs_diff", "mixed"};

    for (auto& csv : datasets) {
        for (double sim : min_sims) {
            for (double beta : betas) {
                for (size_t pop : pop_sizes) {
                    for (size_t gen : generations) {
                        for (double cross : crossovers) {
                            for (double mut : mutations) {
                                if (abs(cross - mut) <= 0.4) continue;
                                for (auto& met : metric_types) {
                                    auto res = RunBenchmark(csv, sim, beta, pop, gen, cross, mut,
                                                            met, 5);
                                    results.push_back(res);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    std::ofstream out("ga_rfd_tuning_results.csv");
    out << "dataset,min_similarity,min_confidence,pop_size,generations,crossover_prob,mutation_"
           "prob,metric_type,time_ms,memory_kb,rfd_count\n";
    std::string sep = ",";
    for (auto& r : results) {
        out << r.dataset << sep << r.min_similarity << sep << r.min_confidence << sep
            << r.population_size << sep << r.max_generations << sep << r.crossover_prob << sep
            << r.mutation_prob << sep << r.metric_type << sep << r.time_ms << sep << r.memory_kb
            << sep << r.rfd_count << "\n";
    }

    std::cout << "Done to ga_rfd_tuning_results.csv\n";
    return 0;
}
