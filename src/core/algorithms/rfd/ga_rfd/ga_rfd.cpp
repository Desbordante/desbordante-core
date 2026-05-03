#include "ga_rfd.h"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"

bool DEBUG = false;

namespace {

template <typename T>
[[nodiscard]] inline bool InRangeInclusive(T value, T min, T max) noexcept {
    return min <= value && value <= max;
}

[[nodiscard]] std::string BitRepresentation(uint32_t mask, int num_bits = 31) {
    return std::bitset<32>(mask).to_string().substr(32 - num_bits);
}

[[nodiscard]] inline int FirstSetBit(uint32_t x) noexcept {
    return x ? __builtin_ctz(x) : -1;
}

}  // namespace

namespace algos::rfd {

std::string RFD::ToString() const {
    std::string res = "[";
    bool first = true;
    for (uint8_t i = 0; i < kMaxAttributes; i++) {
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
    MakeOptionsAvailable({kRfdMinSimilarity, kMinimumConfidence, kPopulationSize,
                          kRfdMaxGenerations, kRfdCrossoverProbability, kRfdMutationProbability,
                          kSeed, "metrics", config::kTableOpt.GetName(), kCacheMaxSize});
}

void GaRfd::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kRfdMinSimilarity, kMinimumConfidence, kPopulationSize,
                          kRfdMaxGenerations, kRfdCrossoverProbability, kRfdMutationProbability,
                          kSeed, "metrics", kCacheMaxSize});
}

void GaRfd::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_prob_range = [](double v) { return InRangeInclusive(v, 0.0, 1.0); };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&metrics_, "metrics", "List of similarity metrics",
                          std::vector<std::shared_ptr<SimilarityMetric>>{}});
    RegisterOption(
            Option{&min_similarity_, kRfdMinSimilarity, kDRfdMinSimilarity, 1.0}.SetValueCheck(
                    check_prob_range));
    RegisterOption(Option{&eps_, kMinimumConfidence, kDMinimumConfidence, 1.0}.SetValueCheck(
            check_prob_range));
    RegisterOption(Option{&population_size_, kPopulationSize, kDPopulationSize,
                          static_cast<std::size_t>(1024)}
                           .SetValueCheck([](auto v) { return v > 0; }));
    RegisterOption(Option{&max_generations_, kRfdMaxGenerations, kDRfdMaxGenerations,
                          static_cast<std::size_t>(32)});
    RegisterOption(Option{&crossover_probability_, kRfdCrossoverProbability,
                          kDRfdCrossoverProbability, 1.0}
                           .SetValueCheck(check_prob_range));
    RegisterOption(
            Option{&mutation_probability_, kRfdMutationProbability, kDRfdMutationProbability, 1.0}
                    .SetValueCheck(check_prob_range));
    RegisterOption(Option{&seed_, kSeed, kDSeed, static_cast<std::uint32_t>(42)});
    RegisterOption(Option{&cache_max_size_, kCacheMaxSize, kDCacheMaxSize,
                          static_cast<std::size_t>(10000)});
}

void GaRfd::LoadDataInternal() {
    num_rows_ = 0;
    column_data_.clear();

    if (!input_table_->HasNextRow()) [[unlikely]]
        throw std::runtime_error("Input table is empty");

    auto first_row = input_table_->GetNextRow();
    num_attrs_ = first_row.size();
    if (num_attrs_ < 2) [[unlikely]]
        throw std::runtime_error("GA-rfd requires at least 2 attributes");
    if (num_attrs_ > kMaxAttributes) [[unlikely]]
        throw std::runtime_error("Maximum 31 attributes supported");
    column_data_.resize(num_attrs_);
    for (size_t i = 0; i < num_attrs_; i++) {
        column_data_[i].push_back(std::move(first_row[i]));
    }
    num_rows_ = 1;

    while (input_table_->HasNextRow()) {
        auto row = input_table_->GetNextRow();
        if (row.size() != num_attrs_) [[unlikely]]
            throw std::runtime_error("Inconsistent number of attributes in row");
        for (size_t i = 0; i < num_attrs_; i++) {
            column_data_[i].push_back(std::move(row[i]));
        }
        num_rows_++;
    }

    if (num_rows_ < 2) [[unlikely]]
        throw std::runtime_error("Input table must contain at least 2 rows");

    if (num_rows_ > std::numeric_limits<std::size_t>::max() / (num_rows_ - 1) / 2) [[unlikely]]
        throw std::runtime_error("Table too large, total pairs would overflow size_t");
    total_pairs_ = num_rows_ * (num_rows_ - 1) / 2;

    if (DEBUG)
        LOG_INFO("Loaded {} rows, {} attributes, {} total pairs", num_rows_, num_attrs_,
                 total_pairs_);

    if (metrics_.empty()) {
        metrics_.clear();
        metrics_.reserve(num_attrs_);
        for (size_t i = 0; i < num_attrs_; i++) metrics_.emplace_back(EqualityMetric());
    }
    if (metrics_.size() != num_attrs_) [[unlikely]]
        throw std::invalid_argument("The number of attributes and metrics do not match");

    support_cache_ = std::make_unique<util::LRUCache<uint32_t, std::size_t>>(cache_max_size_);
}

void GaRfd::BuildSimilarityBitsets() {
    if (!support_cache_) {
        support_cache_ = std::make_unique<util::LRUCache<uint32_t, std::size_t>>(cache_max_size_);
    }
    if (DEBUG)
        LOG_INFO("BuildSimilarityBitsets: total_pairs_ = {}, num_attrs_ = {}, num_rows_ = {}",
                 total_pairs_, num_attrs_, num_rows_);
    std::size_t const num_uint64_per_attr = (total_pairs_ + 63) / 64;
    attr_similarity_bits_.assign(num_attrs_, std::vector<uint64_t>(num_uint64_per_attr, 0));
    if (metrics_.size() != num_attrs_) [[unlikely]]
        throw std::runtime_error("Number of metrics must match number of attributes");

    for (std::size_t a = 0; a < num_attrs_; a++) {
        auto const& col = column_data_[a];
        auto& bits = attr_similarity_bits_[a];
        std::size_t pair_idx = 0;

        for (std::size_t i = 0; i < num_rows_; i++) {
            for (std::size_t j = i + 1; j < num_rows_; j++) {
                try {
                    double const sim = metrics_[a]->Compare(col[i], col[j]);
                    if (sim >= min_similarity_) {
                        std::size_t const word_idx = pair_idx / 64;
                        std::size_t const bit_idx = pair_idx % 64;
                        bits[word_idx] |= (1ULL << bit_idx);
                    }
                } catch (std::exception const& e) {
                    if (DEBUG)
                        LOG_ERROR("Exception in Compare: attr={}, i={}, j={}, what={}", a, i, j,
                                  e.what());
                    throw;
                } catch (...) {
                    if (DEBUG)
                        LOG_ERROR("Unknown exception in Compare: attr={}, i={}, j={}", a, i, j);
                    throw;
                }
                pair_idx++;
            }
        }
        if (DEBUG) LOG_INFO("Finished attribute {} similarity bitset", a);
    }
    if (DEBUG) LOG_INFO("Similarity bitsets built for {} attributes", num_attrs_);
}

std::size_t GaRfd::ComputeSupport(uint32_t attrs_mask) const noexcept {
    if (auto cached = support_cache_->get(attrs_mask)) return *cached;
    if (attrs_mask == 0) [[unlikely]] {
        support_cache_->put(0, total_pairs_);
        return total_pairs_;
    }

    uint32_t mm = attrs_mask;
    int first = FirstSetBit(mm);
    if (first < 0) [[unlikely]] {
        support_cache_->put(attrs_mask, 0);
        return 0;
    }

    auto const& first_vec = attr_similarity_bits_[first];
    if ((attrs_mask & (attrs_mask - 1)) == 0u) {
        std::size_t s = 0;
        for (uint64_t w : first_vec) s += std::popcount(w);
        support_cache_->put(attrs_mask, s);
        if (DEBUG)
            LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attrs_mask, num_attrs_), s);
        return s;
    }

    std::vector<uint64_t> buffer = first_vec;
    mm &= mm - 1;
    while (mm) {
        int a = FirstSetBit(mm);
        auto const& other = attr_similarity_bits_[a];
        std::size_t running = 0;
        for (std::size_t k = 0; k < buffer.size(); k++) {
            buffer[k] &= other[k];
            running += std::popcount(buffer[k]);
        }
        if (running == 0) [[unlikely]] {
            support_cache_->put(attrs_mask, 0);
            return 0;
        }
        mm &= mm - 1;
    }

    std::size_t support = 0;
    for (uint64_t w : buffer) support += std::popcount(w);
    support_cache_->put(attrs_mask, support);
    if (DEBUG)
        LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attrs_mask, num_attrs_), support);
    return support;
}

GaRfd::Individual GaRfd::Evaluate(Individual const& ind) const noexcept {
    uint32_t const lhs_mask = ind.lhs_mask;
    uint8_t const rhs = ind.rhs_index;

    double support_lhs = static_cast<double>(ComputeSupport(lhs_mask)) / total_pairs_;
    if (support_lhs == 0.0) [[unlikely]] {
        return {lhs_mask, rhs, 0.0, 0.0};
    }

    uint32_t const both_mask = lhs_mask | (1u << rhs);
    double support_both = static_cast<double>(ComputeSupport(both_mask)) / total_pairs_;
    double confidence = support_both / support_lhs;
    return {lhs_mask, rhs, confidence, support_both};
}

void GaRfd::EvaluatePopulation(std::unordered_set<Individual, IndividualHash>& pop) const noexcept {
    std::vector<Individual> updated;
    updated.reserve(pop.size());
    for (auto const& ind : pop) {
        updated.push_back(Evaluate(ind));
    }
    pop.clear();
    pop.insert(updated.begin(), updated.end());
}

bool GaRfd::AllOf(std::unordered_set<Individual, IndividualHash> const& pop) const noexcept {
    return !pop.empty() && std::ranges::all_of(pop, [this](Individual const& ind) {
        return ind.confidence >= eps_;
    });
}

double GaRfd::Fitness(double confidence) const noexcept {
    return confidence >= eps_ ? 1.0 : confidence / eps_;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::InitializePopulation(
        std::mt19937& rng) const {
    std::unordered_set<Individual, IndividualHash> pop;
    std::uniform_int_distribution<uint8_t> rhs_dist(0u, num_attrs_ - 1u);
    std::uniform_int_distribution<uint8_t> kdist(1u, num_attrs_ - 1u);

    std::vector<uint8_t> indices(num_attrs_);
    std::iota(indices.begin(), indices.end(), 0);

    std::size_t cnt = 0;
    while (pop.size() < population_size_ && cnt++ < population_size_ * 2 + 1) {
        uint8_t rhs = rhs_dist(rng);
        uint8_t k = kdist(rng);

        std::vector<uint8_t> pool;
        pool.reserve(num_attrs_ - 1);
        for (uint8_t i = 0; i < num_attrs_; i++) {
            if (i != rhs) pool.push_back(i);
        }
        for (uint8_t i = 0; i < k; i++) {
            std::uniform_int_distribution<uint8_t> d(i, static_cast<uint8_t>(pool.size()) - 1u);
            std::swap(pool[i], pool[d(rng)]);
        }
        uint32_t lhs_mask = 0;
        for (size_t i = 0; i < k; i++) lhs_mask |= (1u << pool[i]);

        pop.insert(Individual{lhs_mask, rhs, 0.0, 0.0});
    }
    return pop;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Select(
        std::unordered_set<Individual, IndividualHash> const& pop, std::mt19937& rng) const {
    std::unordered_set<Individual, IndividualHash> selected;
    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    for (auto const& ind : pop) {
        if (dist01(rng) < Fitness(ind.confidence)) {
            selected.insert(ind);
        }
    }
    if (selected.empty()) {
        auto best = *std::max_element(pop.begin(), pop.end(), [](auto const& a, auto const& b) {
            return a.confidence < b.confidence;
        });
        selected.insert(best);
    }
    return selected;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Crossover(
        std::unordered_set<Individual, IndividualHash> const& selected, std::mt19937& rng) const {
    std::unordered_set<Individual, IndividualHash> offspring;
    if (selected.size() < 2) {
        return offspring;
    }

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::bernoulli_distribution coin(0.5);

    for (auto it1 = selected.begin(); it1 != selected.end(); ++it1) {
        auto it2 = it1;
        ++it2;
        for (; it2 != selected.end(); ++it2) {
            if (dist01(rng) >= crossover_probability_) continue;

            Individual const& p1 = *it1;
            Individual const& p2 = *it2;

            uint32_t mask1 = p1.lhs_mask;
            uint32_t mask2 = p2.lhs_mask;
            uint8_t rhs1 = p1.rhs_index;
            uint8_t rhs2 = p2.rhs_index;

            uint32_t diff = mask1 ^ mask2;
            if (diff) {
                int diff_cnt = std::popcount(diff);
                std::uniform_int_distribution<int> cnt_dist(1, diff_cnt);
                int cnt = cnt_dist(rng);
                while (cnt-- > 0) {
                    uint32_t bit = diff & -diff;
                    mask1 ^= bit;
                    mask2 ^= bit;
                    diff &= diff - 1;
                }
            }

            if (coin(rng)) {
                std::swap(rhs1, rhs2);
            }

            if ((mask1 == 0) || (mask1 & (1u << rhs1))) continue;
            if ((mask2 == 0) || (mask2 & (1u << rhs2))) continue;

            offspring.insert(Individual{mask1, rhs1, 0.0, 0.0});
            offspring.insert(Individual{mask2, rhs2, 0.0, 0.0});
        }
    }
    return offspring;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Mutate(
        std::unordered_set<Individual, IndividualHash> const& pop, std::mt19937& rng) const {
    std::unordered_set<Individual, IndividualHash> mutated;
    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::uniform_int_distribution<uint8_t> coin(0, 2);

    for (auto const& ind : pop) {
        if (dist01(rng) >= mutation_probability_) {
            mutated.insert(ind);
            continue;
        }

        uint32_t mask = ind.lhs_mask;
        uint8_t rhs = ind.rhs_index;

        bool mutated_flag = false;
        switch (coin(rng)) {
            case 0: {
                int ones = std::popcount(mask);
                if (ones == 0) break;
                int skip = std::uniform_int_distribution<int>(0, ones - 1)(rng);
                uint32_t m = mask;
                while (skip--) m &= m - 1;
                mask ^= (m & -m);
                mutated_flag = true;
                break;
            }
            case 1: {
                uint32_t avail = ((1u << num_attrs_) - 1) & ~mask & ~(1u << rhs);
                if (avail == 0) break;
                int ones = std::popcount(avail);
                int skip = std::uniform_int_distribution<int>(0, ones - 1)(rng);
                uint32_t m = avail;
                while (skip--) m &= m - 1;
                mask |= (m & -m);
                mutated_flag = true;
                break;
            }
            case 2: {
                uint32_t avail = ((1u << num_attrs_) - 1) & ~mask;
                if (avail == 0) break;
                int ones = std::popcount(avail);
                int skip = std::uniform_int_distribution<int>(0, ones - 1)(rng);
                uint32_t m = avail;
                while (skip--) m &= m - 1;
                rhs = static_cast<uint8_t>(__builtin_ctz(m & -m));
                mutated_flag = true;
                break;
            }
        }
        if (mutated_flag && mask != 0 && !(mask & (1u << rhs)))
            mutated.insert(Individual{mask, rhs, 0.0, 0.0});
        else
            mutated.insert(ind);
    }
    return mutated;
}

std::unordered_set<RFD, RFDHash> GaRfd::Finalize(
        std::unordered_set<Individual, IndividualHash> const& pop) const {
    std::unordered_set<RFD, RFDHash> res;
    for (auto const& ind : pop) {
        if (ind.confidence < eps_) continue;
        res.insert(RFD{ind.lhs_mask, ind.rhs_index, ind.support, ind.confidence});
    }
    if (DEBUG) LOG_DEBUG("Finalized {} unique RFDs", res.size());
    return res;
}

unsigned long long GaRfd::ExecuteInternal() {
    return ::util::TimedInvoke([&]() {
        if (DEBUG) LOG_INFO("Build similarity bitsets...");
        BuildSimilarityBitsets();

        std::mt19937 rng(seed_);

        auto pop = InitializePopulation(rng);
        EvaluatePopulation(pop);

        for (size_t gen = 0; gen < max_generations_; gen++) {
            if (DEBUG)
                LOG_INFO("Generation {}/{} (pop size: {})", gen + 1, max_generations_, pop.size());
            if (AllOf(pop)) {
                if (DEBUG)
                    LOG_DEBUG("All individuals satisfy confidence threshold – stopping early");
                break;
            }

            if (pop.empty()) [[unlikely]] {
                if (DEBUG) LOG_DEBUG("Population is empty, stopping evolution");
                break;
            }

            auto selected = Select(pop, rng);
            auto offspring = Crossover(selected, rng);
            auto mutated = Mutate(selected, rng);

            if (offspring.empty() && mutated.empty()) {
                pop = std::move(selected);
            } else {
                pop = std::move(selected);
                pop.insert(offspring.begin(), offspring.end());
                pop.insert(mutated.begin(), mutated.end());
            }

            if (pop.size() > population_size_ + 100) {
                std::vector<Individual> sorted(pop.begin(), pop.end());
                std::sort(sorted.begin(), sorted.end(),
                          [](auto const& a, auto const& b) { return a.confidence > b.confidence; });
                sorted.resize(population_size_ + 100);
                pop = std::unordered_set<Individual, IndividualHash>(sorted.begin(), sorted.end());
            }

            EvaluatePopulation(pop);
        }

        discovered_ = Finalize(pop);
    });
}

void GaRfd::ResetState() {
    discovered_.clear();
    support_cache_.reset();
}

GaRfd::~GaRfd() {
    ResetState();
}

}  // namespace algos::rfd
