#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ranges>
#include <stdexcept>
#include <unordered_set>
#include <set>

#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"

namespace {

template <typename T>
inline bool InRangeInclusive(T value, T min, T max) {
    return min <= value && value <= max;
}

inline uint32_t popcount(uint32_t x) {
    return std::popcount(x);
}

inline uint8_t countr_zero(uint32_t x) {
    if (x == 0) return 32;
    return __builtin_ctz(x);
}

}  // namespace

namespace algos::rfd {

std::string RFD::ToString() const {
    std::string res = "[";
    bool first = true;
    for (uint8_t i = 0; i < 31; ++i) {
        if (lhs_mask & (1u << i)) {
            if (!first) res += ", ";
            res += std::to_string(i);
            first = false;
        }
    }
    res += "] -> " + std::to_string(rhs_index) + " (conf=" + std::to_string(confidence) +
           ", supp=" + std::to_string(support) + ")";
    return res;
}

GaRfd::GaRfd() : Algorithm() {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kRfdMinSimilarity,
                          kMinimumConfidence,
                          kPopulationSize,
                          kRfdMaxGenerations,
                          kRfdCrossoverProbability,
                          kRfdMutationProbability,
                          kSeed,
                          "metrics",
                          config::kTableOpt.GetName()});
}

void GaRfd::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kRfdMinSimilarity,
                          kMinimumConfidence,
                          kPopulationSize,
                          kRfdMaxGenerations,
                          kRfdCrossoverProbability,
                          kRfdMutationProbability,
                          kSeed,
                          "metrics"});
}

void GaRfd::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_prob_range = [](double v) { return InRangeInclusive(v, 0.0, 1.0); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&metrics_, 
                          "metrics",
                          "List of similarity metrics",
                          std::vector<std::shared_ptr<SimilarityMetric>>{}});
    RegisterOption(Option{&min_similarity_,
                          kRfdMinSimilarity,
                          kDRfdMinSimilarity,
                          0.7}.SetValueCheck(check_prob_range));
    RegisterOption(Option{&beta_,
                          kMinimumConfidence,
                          kDMinimumConfidence,
                          0.75}.SetValueCheck(check_prob_range));
    RegisterOption(Option{&population_size_,
                          kPopulationSize,
                          kDPopulationSize,
                          static_cast<std::size_t>(20)}.SetValueCheck([](auto v) { return v > 0; }));
    RegisterOption(Option{&max_generations_,
                          kRfdMaxGenerations,
                          kDRfdMaxGenerations,
                          static_cast<std::size_t>(50)});
    RegisterOption(Option{&crossover_probability_,
                          kRfdCrossoverProbability,
                          kDRfdCrossoverProbability,
                          0.85}.SetValueCheck(check_prob_range));
    RegisterOption(Option{&mutation_probability_,
                          kRfdMutationProbability,
                          kDRfdMutationProbability,
                          0.3}.SetValueCheck(check_prob_range));
    RegisterOption(Option{&seed_,
                          kSeed,
                          kDSeed,
                          static_cast<std::uint64_t>(42)});
}

void GaRfd::LoadDataInternal() {
    std::vector<std::vector<std::string>> temp_data;
    while (input_table_->HasNextRow())
        temp_data.push_back(input_table_->GetNextRow());

    num_rows_ = temp_data.size(); // 0 < num_rows_ < sqrt(SIZE_MAX)
    if (num_rows_ == 0) throw std::runtime_error("Input table is empty");
    num_attrs_ = temp_data[0].size();
    if (num_attrs_ < 2) throw std::runtime_error("GA-rfd requires at least 2 attributes");
    if (num_attrs_ > 31) throw std::runtime_error("Maximum 31 attributes supported");
    total_pairs_ = num_rows_ * (num_rows_ - 1) / 2;

    LOG_DEBUG("Loaded {} rows, {} attributes, {} total pairs", num_rows_, num_attrs_, total_pairs_);

    column_data_.resize(num_attrs_);
    for (std::size_t i = 0; i < num_attrs_; i++) {
        column_data_[i].resize(num_rows_);
        for (std::size_t j = 0; j < num_rows_; j++) {
            column_data_[i][j] = std::move(temp_data[j][i]);
        }
    }

    if (metrics_.empty())
        metrics_.assign(num_attrs_, EqualityMetric());
    if (metrics_.size() != num_attrs_)
        throw std::invalid_argument("The number of attributes and metrics do not match");
}

void GaRfd::BuildSimilarityBitsets() {
    LOG_DEBUG("BuildSimilarityBitsets: total_pairs_ = {}, num_attrs_ = {}, num_rows_ = {}",
              total_pairs_, num_attrs_, num_rows_);
    std::size_t num_uint64_per_attr = (total_pairs_ + 63) / 64;
    attr_similarity_bits_.assign(num_attrs_, std::vector<uint64_t>(num_uint64_per_attr, 0));
    if (metrics_.size() != num_attrs_)
        throw std::runtime_error("Number of metrics must match number of attributes");

    for (std::size_t a = 0; a < num_attrs_; a++) {
        const auto& col = column_data_[a];
        auto& bits = attr_similarity_bits_[a];
        std::size_t pair_idx = 0;

        for (std::size_t i = 0; i < num_rows_; i++) {
            for (std::size_t j = i+1; j < num_rows_; j++) {
                try {
                    double sim = metrics_[a]->Compare(col[i], col[j]);
                    if (sim >= min_similarity_) {
                        std::size_t word_idx = pair_idx / 64;
                        std::size_t bit_idx = pair_idx % 64;
                        bits[word_idx] |= (1ULL << bit_idx);
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in Compare: attr={}, i={}, j={}, what={}", a, i, j, e.what());
                    throw;
                } catch (...) {
                    LOG_ERROR("Unknown exception in Compare: attr={}, i={}, j={}", a, i, j);
                    throw;
                }
                pair_idx++;
            }
        }
        LOG_DEBUG("Finished attribute {} similarity bitset", a);
    }
    LOG_DEBUG("Similarity bitsets built for {} attributes", num_attrs_);
}

std::size_t GaRfd::ComputeSupport(uint32_t attrs_mask) const {
    if (auto cached = support_cache_.get(attrs_mask)) {
        return *cached;
    }

    if (attrs_mask == 0) [[unlikely]] {
        support_cache_.put(0, total_pairs_);
        return total_pairs_;
    }

    int first = countr_zero(attrs_mask);
    std::vector<uint64_t> intersection = attr_similarity_bits_[first];
    uint32_t remaining = attrs_mask & (attrs_mask - 1);

    while (remaining) {
        int attr = countr_zero(remaining);
        const auto& other_bits = attr_similarity_bits_[attr];
        for (std::size_t k = 0; k < intersection.size(); ++k) {
            intersection[k] &= other_bits[k];
        }
        remaining &= remaining - 1;
    }

    std::size_t support = 0;
    for (uint64_t word : intersection) {
        support += std::popcount(word);
    }
    support_cache_.put(attrs_mask, support);

    LOG_DEBUG("Support for mask {} = {}", attrs_mask, support);

    return support;
}

GaRfd::Individual GaRfd::Evaluate(const Individual& ind) const {
    uint32_t lhs_mask = ind.lhs_mask;
    uint8_t rhs = ind.rhs_index;

    double support_lhs = static_cast<double>(ComputeSupport(lhs_mask)) / total_pairs_;
    if (support_lhs == 0.0) [[unlikely]] {
        return {lhs_mask, rhs, 0.0, 0.0};
    }

    uint32_t both_mask = lhs_mask | (1u << rhs);
    double support_both = static_cast<double>(ComputeSupport(both_mask)) / total_pairs_;
    double confidence = support_both / support_lhs;
    return {lhs_mask, rhs, confidence, support_both};
}

void GaRfd::EvaluatePopulation(std::vector<Individual>& pop) const {
    for (auto& ind : pop) ind = Evaluate(ind);
}

bool GaRfd::AllOf(std::vector<Individual> const& pop) const {
    if (pop.empty()) [[unlikely]] return false;
    return std::ranges::all_of(pop, [&](Individual const& ind) { return ind.confidence >= beta_; });
}

double GaRfd::Fitness(double confidence) const {
    return (confidence >= beta_) ? 1.0 : confidence / beta_;
}

std::vector<GaRfd::Individual> GaRfd::InitializePopulation(std::mt19937_64& rng) const {
    std::vector<Individual> pop;
    pop.reserve(population_size_);

    std::uniform_int_distribution<uint32_t> mask_dist(1u, (1u << num_attrs_) - 1);
    std::uniform_int_distribution<uint8_t> rhs_dist(0, num_attrs_ - 1);

    while (pop.size() < population_size_) {
        uint32_t lhs_mask = mask_dist(rng);
        uint8_t rhs = rhs_dist(rng);
        if ((lhs_mask & (1u << rhs)) || lhs_mask == 0u) continue;
        pop.push_back(Individual{lhs_mask, rhs, 0.0, 0.0});
    }
    LOG_DEBUG("Initial population of size {} created", pop.size());

    return pop;
}

std::vector<GaRfd::Individual> GaRfd::Select(std::vector<Individual> const& pop,
                                             std::mt19937_64& rng) const {
    std::vector<Individual> selected;
    selected.reserve(pop.size());

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    for (auto const& ind : pop) {
        if (dist01(rng) < Fitness(ind.confidence)) selected.push_back(ind);
    }
    return selected;
}

std::vector<GaRfd::Individual> GaRfd::Crossover(std::vector<Individual> const& selected,
                                                std::mt19937_64& rng) const {
    std::vector<Individual> offspring;
    if (selected.size() < 2) return offspring;

    std::uniform_real_distribution<double> dist01(0.0, 1.0);

    for (std::size_t i = 0; i < selected.size(); ++i) {
        for (std::size_t j = i + 1; j < selected.size(); ++j) {
            if (dist01(rng) >= crossover_probability_) continue;

            Individual c1 = selected[i];
            Individual c2 = selected[j];

            uint32_t mask1 = c1.lhs_mask;
            uint32_t mask2 = c2.lhs_mask;
            uint32_t diff = mask1 ^ mask2;
            auto cnts_of_ones = (std::popcount(diff) > 1 ? std::popcount(diff)-1 : 2);
            std::uniform_int_distribution<uint8_t> until(1, cnts_of_ones);
            uint8_t cnt = until(rng);
            while (diff && cnt--) {
                uint32_t bit = diff & -diff;
                mask1 ^= bit;
                mask2 ^= bit;
            }

            c1.lhs_mask = mask1;
            c2.lhs_mask = mask2;

            if (until(rng) == 1) {
                std::swap(c1.rhs_index, c2.rhs_index);
            }

            if ((c1.lhs_mask & (1u << c1.rhs_index)) || c1.lhs_mask == 0) continue;
            if ((c2.lhs_mask & (1u << c2.rhs_index)) || c2.lhs_mask == 0) continue;

            c1.confidence = 0.0;
            c1.support = 0.0;
            c2.confidence = 0.0;
            c2.support = 0.0;

            offspring.push_back(c1);
            offspring.push_back(c2);
        }
    }
    return offspring;
}

void GaRfd::Mutate(std::vector<Individual>& pop, std::mt19937_64& rng) const {
    if (pop.empty()) return;

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::uniform_int_distribution<uint8_t> rhs_dist(0, num_attrs_ - 1);
    std::uniform_int_distribution<uint8_t> coin(0, 2);

    for (auto& ind : pop) {
        if (dist01(rng) >= mutation_probability_) continue;

        uint32_t mask = ind.lhs_mask;
        uint8_t rhs = ind.rhs_index;

        switch (coin(rng)) {
            case 0:
                if (mask) {
                    uint32_t bit = 1u << (countr_zero(mask));
                    mask ^= bit;
                }
                break;
            case 1:
                if ((mask | (1u << rhs)) != ((1u << num_attrs_) - 1)) {
                    uint32_t new_bit = 1u << rhs_dist(rng);
                    while (mask & new_bit) new_bit = 1u << rhs_dist(rng);
                    mask |= new_bit;
                }
                break;
            case 2:
                rhs = rhs_dist(rng);
                while (mask & (1u << rhs)) rhs = rhs_dist(rng);
                break;
        }

        if (mask == 0 || (mask & (1u << rhs))) continue;

        ind.lhs_mask = mask;
        ind.rhs_index = rhs;
        ind.confidence = 0.0;
        ind.support = 0.0;
    }
}

std::set<RFD> GaRfd::Finalize(std::vector<Individual> const& pop) const {
    std::set<RFD> res;
    for (auto const& ind : pop) {
        if (ind.confidence < beta_) continue;

        RFD rfd;
        rfd.lhs_mask = ind.lhs_mask;
        rfd.rhs_index = ind.rhs_index;
        rfd.support = ind.support;
        rfd.confidence = ind.confidence;

        res.insert(rfd);
    }
    LOG_DEBUG("Finalized {} unique RFDs", res.size());

    return res;
}

unsigned long long GaRfd::ExecuteInternal() {
    return util::TimedInvoke([&]() {
        LOG_DEBUG("Build similarity bitsets...");
        BuildSimilarityBitsets();
        support_cache_.clear();

        std::mt19937_64 rng(seed_);

        std::vector<Individual> pop = InitializePopulation(rng);
        EvaluatePopulation(pop);

        for (std::size_t gen = 0; gen <= max_generations_; gen++) {
            if (AllOf(pop)) break;

            std::vector<Individual> selected = Select(pop, rng);
            std::vector<Individual> offspring = Crossover(selected, rng);
            Mutate(offspring, rng);

            pop.clear();
            pop.reserve(selected.size() + offspring.size());
            pop.insert(pop.end(), selected.begin(), selected.end());
            pop.insert(pop.end(), offspring.begin(), offspring.end());

            EvaluatePopulation(pop);
        }

        discovered_ = Finalize(pop);
    });
}

}  // namespace algos::rfd
