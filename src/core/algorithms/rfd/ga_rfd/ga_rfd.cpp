#include "ga_rfd.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <stdexcept>

#include "core/config/names_and_descriptions.h"
#include "core/util/logger.h"

namespace algos::rfd {

GaRfd::GaRfd() : Algorithm(), rng_(seed_) {
    using namespace config::names;

    RegisterOption(config::Option{&table_, kTable, "Input table"});
    RegisterOption(config::Option{&metrics_, kMetrics, "List of similarity metrics"});
    RegisterOption(config::Option{&similarity_thresholds_, kSimilarityThresholds,
                                  "Thresholds for each attribute"});
    RegisterOption(
        config::Option{&min_confidence_, kMinConfidence, "Minimum confidence"});
    RegisterOption(
        config::Option{&population_size_, kPopulationSize, "Population size"});
    RegisterOption(
        config::Option{&max_generations_, kMaxGenerations, "Maximum generations"});
    RegisterOption(
        config::Option{&crossover_prob_, kCrossoverProb, "Crossover probability"});
    RegisterOption(
        config::Option{&mutation_prob_, kMutationProb, "Mutation probability"});
    RegisterOption(config::Option{&max_lhs_arity_, kMaxLhsArity, "Max LHS arity"});
    RegisterOption(config::Option{&seed_, kSeed, "Random seed"});
    RegisterOption(config::Option{&output_file_, kOutputFile, "Output file path"});

    MakeOptionsAvailable({kTable, kMetrics, kSimilarityThresholds, kMinConfidence,
                          kPopulationSize, kMaxGenerations, kCrossoverProb, kMutationProb,
                          kMaxLhsArity, kSeed, kOutputFile});
}

void GaRfd::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({
        kMetrics,
        kSimilarityThresholds,
        kMinConfidence,
        kPopulationSize,
        kMaxGenerations,
        kCrossoverProb,
        kMutationProb,
        kMaxLhsArity,
        kSeed,
        kOutputFile,
    });
}

void GaRfd::LoadDataInternal() {
    table_->Reset();
    data_.clear();
    while (table_->HasNextRow()) {
        data_.push_back(table_->GetNextRow());
    }

    num_rows_ = data_.size();
    if (num_rows_ == 0) {
        throw std::runtime_error("Input table is empty");
    }
    num_attrs_ = data_[0].size();
    total_pairs_ = num_rows_ * num_rows_;

    if (metrics_.empty()) {
        metrics_.assign(num_attrs_, LevenshteinMetric());
    }
    if (metrics_.size() != num_attrs_) {
        throw std::invalid_argument("Number of metrics must match number of attributes");
    }
    if (similarity_thresholds_.size() != num_attrs_) {
        throw std::invalid_argument(
            "Number of similarity thresholds must match number of attributes");
    }

    LOG_INFO("GA-RFD data loaded: {} rows, {} attributes, {} total pairs",
             num_rows_, num_attrs_, total_pairs_);
}

unsigned long long GaRfd::ExecuteInternal() {
    auto start = std::chrono::steady_clock::now();

    BuildSimilarityBitsets();
    support_cache_.clear();
    support_cache_.reserve(1024);

    auto population = InitializePopulation();
    EvaluatePopulation(population);

    for (size_t gen = 0; gen < max_generations_; ++gen) {
        bool all_valid = std::all_of(population.begin(), population.end(),
            [this](const Individual& ind) {
                return ind.fitness >= min_confidence_;
            });
        if (all_valid) break;

        auto selected = Selection(population);
        auto offspring = Crossover(selected);
        offspring = Mutation(std::move(offspring));
        EvaluatePopulation(offspring);

        // Elitism
        std::sort(population.begin(), population.end(),
            [](const Individual& a, const Individual& b) {
                return a.fitness > b.fitness;
            });
        size_t elite_count = std::max<size_t>(1, population.size() / 10);
        std::vector<Individual> new_population;
        new_population.insert(new_population.end(),
                              population.begin(),
                              population.begin() + elite_count);
        new_population.insert(new_population.end(),
                              offspring.begin(),
                              offspring.end());

        // Fill with random if needed
        while (new_population.size() < population_size_) {
            Individual rand_ind;
            rand_ind.genes.resize(max_lhs_arity_ + 1);
            std::uniform_int_distribution<size_t> attr_dist(0, num_attrs_ - 1);
            for (auto& g : rand_ind.genes) g = attr_dist(rng_);
            new_population.push_back(std::move(rand_ind));
        }
        new_population.resize(population_size_);
        population = std::move(new_population);
    }

    RegisterResults(population);

    if (!output_file_.empty()) {
        SaveResults(output_file_);
    }

    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void GaRfd::ResetState() {
    support_cache_.clear();
    attr_bitsets_.clear();
    discovered_rfds_.clear();
    rng_.seed(seed_);
}

std::vector<std::string> GaRfd::GetResultStrings() const {
    std::vector<std::string> result;
    result.reserve(discovered_rfds_.size());
    for (const auto& ind : discovered_rfds_) {
        std::string s = "[";
        for (size_t i = 0; i < ind.genes.size() - 1; ++i) {
            s += std::to_string(ind.genes[i]) + " ";
        }
        s += "] -> " + std::to_string(ind.genes.back()) +
             " (conf=" + std::to_string(ind.fitness) + ")";
        result.push_back(s);
    }
    return result;
}

void GaRfd::SaveResults(const std::string& filepath) const {
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) return;
    for (const auto& line : GetResultStrings()) {
        ofs << line << '\n';
    }
}

// ========== GA methods ==========

void GaRfd::BuildSimilarityBitsets() {
    attr_bitsets_.resize(num_attrs_, boost::dynamic_bitset<>(total_pairs_));

    size_t pair_idx = 0;
    for (size_t i = 0; i < num_rows_; ++i) {
        for (size_t j = 0; j < num_rows_; ++j) {
            for (size_t a = 0; a < num_attrs_; ++a) {
                double sim = metrics_[a]->Compare(data_[i][a], data_[j][a]);
                if (sim >= similarity_thresholds_[a]) {
                    attr_bitsets_[a].set(pair_idx);
                }
            }
            ++pair_idx;
        }
    }
}

namespace {
int first_bit_set(uint64_t mask) {
    if (mask == 0) return -1;
    return std::countr_zero(mask);
}
}  // namespace

size_t GaRfd::ComputeSupport(uint64_t attrs_mask) const {
    auto it = support_cache_.find(attrs_mask);
    if (it != support_cache_.end()) return it->second;

    if (attrs_mask == 0) {
        support_cache_[0] = total_pairs_;
        return total_pairs_;
    }

    int first = first_bit_set(attrs_mask);
    boost::dynamic_bitset<> intersection = attr_bitsets_[first];
    uint64_t remaining = attrs_mask ^ (1ULL << first);
    while (remaining) {
        int attr = first_bit_set(remaining);
        intersection &= attr_bitsets_[attr];
        remaining ^= (1ULL << attr);
        if (intersection.none()) break;
    }
    size_t support = intersection.count();
    support_cache_[attrs_mask] = support;
    return support;
}

double GaRfd::CalculateConfidence(uint64_t lhs_mask, size_t rhs) {
    size_t count_lhs = ComputeSupport(lhs_mask);
    if (count_lhs == 0) return 0.0;
    uint64_t lhs_rhs_mask = lhs_mask | (1ULL << rhs);
    size_t count_both = ComputeSupport(lhs_rhs_mask);
    return static_cast<double>(count_both) / count_lhs;
}

std::vector<GaRfd::Individual> GaRfd::InitializePopulation() {
    std::vector<Individual> pop(population_size_);
    std::uniform_int_distribution<size_t> attr_dist(0, num_attrs_ - 1);
    for (auto& ind : pop) {
        ind.genes.resize(max_lhs_arity_ + 1);
        for (size_t i = 0; i < ind.genes.size(); ++i) {
            ind.genes[i] = attr_dist(rng_);
        }
        ind.fitness = 0.0;
    }
    return pop;
}

void GaRfd::EvaluatePopulation(std::vector<Individual>& population) {
    for (auto& ind : population) {
        std::vector<size_t> lhs(ind.genes.begin(), ind.genes.end() - 1);
        std::sort(lhs.begin(), lhs.end());
        lhs.erase(std::unique(lhs.begin(), lhs.end()), lhs.end());
        size_t rhs = ind.genes.back();

        uint64_t lhs_mask = 0;
        for (size_t a : lhs) lhs_mask |= (1ULL << a);
        ind.fitness = CalculateConfidence(lhs_mask, rhs);
    }
}

std::vector<GaRfd::Individual> GaRfd::Selection(const std::vector<Individual>& population) {
    std::vector<Individual> selected;
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    for (const auto& ind : population) {
        double p = (ind.fitness >= min_confidence_)
                       ? 1.0
                       : ind.fitness / min_confidence_;
        if (prob_dist(rng_) < p) {
            selected.push_back(ind);
        }
    }
    if (selected.empty()) {
        auto best = std::max_element(population.begin(), population.end(),
            [](const Individual& a, const Individual& b) {
                return a.fitness < b.fitness;
            });
        selected.push_back(*best);
    }
    return selected;
}

std::vector<GaRfd::Individual> GaRfd::Crossover(const std::vector<Individual>& parents) {
    std::vector<Individual> offspring;
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<size_t> point_dist(1, max_lhs_arity_);
    for (size_t i = 0; i + 1 < parents.size(); i += 2) {
        const auto& p1 = parents[i];
        const auto& p2 = parents[i + 1];
        if (prob_dist(rng_) < crossover_prob_) {
            size_t cp = point_dist(rng_);
            Individual c1, c2;
            for (size_t j = 0; j < cp; ++j) {
                c1.genes.push_back(p1.genes[j]);
                c2.genes.push_back(p2.genes[j]);
            }
            for (size_t j = cp; j < p1.genes.size(); ++j) {
                c1.genes.push_back(p2.genes[j]);
                c2.genes.push_back(p1.genes[j]);
            }
            offspring.push_back(std::move(c1));
            offspring.push_back(std::move(c2));
        } else {
            offspring.push_back(p1);
            offspring.push_back(p2);
        }
    }
    return offspring;
}

std::vector<GaRfd::Individual> GaRfd::Mutation(std::vector<Individual> individuals) {
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    std::uniform_int_distribution<size_t> attr_dist(0, num_attrs_ - 1);
    std::uniform_int_distribution<size_t> gene_dist(0, max_lhs_arity_);
    for (auto& ind : individuals) {
        if (prob_dist(rng_) < mutation_prob_) {
            size_t pos = gene_dist(rng_);
            if (pos < ind.genes.size()) {
                ind.genes[pos] = attr_dist(rng_);
            }
        }
    }
    return individuals;
}

void GaRfd::RegisterResults(const std::vector<Individual>& final_population) {
    discovered_rfds_.clear();
    for (const auto& ind : final_population) {
        if (ind.fitness >= min_confidence_) {
            discovered_rfds_.push_back(ind);
        }
    }
}

}  // namespace algos::rfd
