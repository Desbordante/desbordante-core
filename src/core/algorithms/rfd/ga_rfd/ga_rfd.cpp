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
    MakeOptionsAvailable({kRfdMinSimilarity, kRfdMinimumConfidence, kPopulationSize,
                          kRfdMaxGenerations, kRfdCrossoverProbability, kRfdMutationProbability,
                          kSeed, kMetrics, config::kTableOpt.GetName(), kCacheMaxSize});
}

void GaRfd::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kRfdMinSimilarity, kRfdMinimumConfidence, kPopulationSize,
                          kRfdMaxGenerations, kRfdCrossoverProbability, kRfdMutationProbability,
                          kSeed, kMetrics, kCacheMaxSize});
}

void GaRfd::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto check_prob_range = [](double v) { return InRangeInclusive(v, 0.0, 1.0); };
    auto check_sim = [check_prob_range](auto const& vec) {
        return std::ranges::all_of(vec, check_prob_range);
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&metrics_, kMetrics, kDRfdMetrics,
                          std::vector<std::shared_ptr<SimilarityMetric>>{}});
    RegisterOption(Option{&min_similarity_, kRfdMinSimilarity, kDRfdMinSimilarity,
                          std::vector<double>{1.0}}
                           .SetValueCheck(check_sim));
    RegisterOption(Option{&eps_, kRfdMinimumConfidence, kDRfdMinimumConfidence, 1.0}.SetValueCheck(
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
    RegisterOption(
            Option{&cache_max_size_, kCacheMaxSize, kDCacheMaxSize, static_cast<std::size_t>(10000)}
                    .SetValueCheck([](auto v) { return v >= 0; }));
}

void GaRfd::LoadDataInternal() {
    num_rows_ = 0;
    column_data_.clear();

    if (!input_table_->HasNextRow()) throw std::runtime_error("Input table is empty");

    auto first_row = input_table_->GetNextRow();
    num_attrs_ = first_row.size();
    full_mask_ = (1u << num_attrs_) - 1;
    if (num_attrs_ < 2) throw std::runtime_error("GA-rfd requires at least 2 attributes");
    if (num_attrs_ > kMaxAttributes) throw std::runtime_error("Maximum 31 attributes supported");

    constexpr size_t k_reserve_chunk = 1024;
    column_data_.resize(num_attrs_);
    for (auto& col : column_data_) col.reserve(k_reserve_chunk);

    for (size_t i = 0; i < num_attrs_; ++i) column_data_[i].emplace_back(std::move(first_row[i]));
    num_rows_ = 1;

    while (input_table_->HasNextRow()) {
        auto row = input_table_->GetNextRow();
        assert(row.size() == num_attrs_ && "Inconsistent number of attributes");
        for (size_t i = 0; i < num_attrs_; ++i) column_data_[i].emplace_back(std::move(row[i]));
        ++num_rows_;
    }

    if (num_rows_ < 2) throw std::runtime_error("Input table must contain at least 2 rows");

    if (num_rows_ > std::numeric_limits<std::size_t>::max() / (num_rows_ - 1) / 2)
        throw std::runtime_error("Table too large, total pairs would overflow size_t");
    total_pairs_ = num_rows_ * (num_rows_ - 1) / 2;

    LOG_INFO("Loaded {} rows, {} attributes, {} total pairs", num_rows_, num_attrs_, total_pairs_);

    if (min_similarity_.empty()) {
        min_similarity_.assign(num_attrs_, 1.0);
    } else if (min_similarity_.size() == 1) {
        min_similarity_.assign(num_attrs_, min_similarity_[0]);
    } else if (min_similarity_.size() != num_attrs_) {
        throw std::invalid_argument("min_similarity size must match the number of attributes");
    }

    if (metrics_.empty()) {
        metrics_.clear();
        metrics_.reserve(num_attrs_);
        for (size_t i = 0; i < num_attrs_; i++) metrics_.emplace_back(EqualityMetric());
    }
    if (metrics_.size() != num_attrs_)
        throw std::invalid_argument("The number of attributes and metrics do not match");

    support_cache_ = std::make_unique<util::LRUCache<uint32_t, std::size_t>>(cache_max_size_);
}

void GaRfd::BuildSimilarityBitsets() {
    if (!support_cache_) {
        support_cache_ = std::make_unique<util::LRUCache<uint32_t, std::size_t>>(cache_max_size_);
    }
    LOG_INFO("BuildSimilarityBitsets: total_pairs_ = {}, num_attrs_ = {}, num_rows_ = {}",
             total_pairs_, num_attrs_, num_rows_);
    std::size_t const num_uint64_per_attr = (total_pairs_ + 63) / 64;
    attr_similarity_bits_.assign(num_attrs_, std::vector<uint64_t>(num_uint64_per_attr, 0));

    for (size_t a = 0; a < num_attrs_; ++a) {
        auto const& col = column_data_[a];
        auto& bits = attr_similarity_bits_[a];
        double const min_sim = min_similarity_[a];
        auto const& metric = *metrics_[a];

        uint64_t mask = 1;
        size_t word_idx = 0;

        for (size_t i = 0; i < num_rows_; ++i) {
            auto const& val_i = col[i];
            for (size_t j = i + 1; j < num_rows_; ++j) {
                if (metric.Compare(val_i, col[j]) >= min_sim) {
                    bits[word_idx] |= mask;
                }
                mask <<= 1;
                if (__builtin_expect(mask == 0, 0)) {
                    mask = 1;
                    ++word_idx;
                }
            }
        }
        LOG_INFO("Finished attribute {} similarity bitset", a);
    }
    LOG_INFO("Similarity bitsets built for {} attributes", num_attrs_);
}

std::size_t GaRfd::ComputeSupport(uint32_t attrs_mask) const {
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

    if (first_vec.empty()) [[unlikely]] {
        support_cache_->put(attrs_mask, 0);
        return 0;
    }

    if ((attrs_mask & (attrs_mask - 1)) == 0u) {
        std::size_t s = 0;
        for (uint64_t w : first_vec) s += std::popcount(w);
        support_cache_->put(attrs_mask, s);
        LOG_INFO("Support for mask {} = {}", BitRepresentation(attrs_mask, num_attrs_), s);
        return s;
    }

    std::size_t const vec_size = first_vec.size();
    if (compute_buffer_.size() != vec_size) compute_buffer_.resize(vec_size);

    std::memcpy(compute_buffer_.data(), first_vec.data(), vec_size * sizeof(uint64_t));

    mm &= mm - 1;
    while (mm) {
        int a = FirstSetBit(mm);
        auto const& other = attr_similarity_bits_[a];
        std::size_t running = 0;
        for (std::size_t k = 0; k < vec_size; ++k) {
            compute_buffer_[k] &= other[k];
            running += std::popcount(compute_buffer_[k]);
        }
        if (running == 0) [[unlikely]] {
            support_cache_->put(attrs_mask, 0);
            return 0;
        }
        mm &= mm - 1;
    }

    std::size_t support = 0;
    for (std::size_t k = 0; k < vec_size; ++k) support += std::popcount(compute_buffer_[k]);

    support_cache_->put(attrs_mask, support);
    LOG_INFO("Support for mask {} = {}", BitRepresentation(attrs_mask, num_attrs_), support);
    return support;
}

GaRfd::Individual GaRfd::Evaluate(Individual const& ind) const {
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

void GaRfd::EvaluatePopulation(std::unordered_set<Individual, IndividualHash>& pop) const {
    auto it = pop.begin();
    while (it != pop.end()) {
        auto node = pop.extract(it++);
        if (!node.empty()) {
            node.value() = Evaluate(node.value());
            pop.insert(std::move(node));
        }
    }
}

bool GaRfd::AllOf(std::unordered_set<Individual, IndividualHash> const& pop) const {
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
    pop.reserve(population_size_);

    std::uniform_int_distribution<uint8_t> rhs_dist(0, num_attrs_ - 1);
    std::uniform_int_distribution<uint8_t> k_dist(1, num_attrs_ - 1);
    std::uniform_int_distribution<uint8_t> shuffle_dist;

    std::vector<uint8_t> all_indices(num_attrs_);
    std::iota(all_indices.begin(), all_indices.end(), 0);

    std::vector<uint8_t> pool(num_attrs_);
    uint8_t const effective_last = static_cast<uint8_t>(num_attrs_ - 1);

    std::size_t cnt = 0;
    while (pop.size() < population_size_ && cnt++ < population_size_ * 2 + 1) {
        uint8_t const rhs = rhs_dist(rng);
        uint8_t const k = k_dist(rng);

        std::memcpy(pool.data(), all_indices.data(), num_attrs_ * sizeof(uint8_t));

        std::swap(pool[rhs], pool[effective_last]);

        for (uint8_t i = 0; i < k; ++i) {
            using param_t = std::uniform_int_distribution<uint8_t>::param_type;
            uint8_t j = shuffle_dist(rng, param_t(i, effective_last - 1));
            std::swap(pool[i], pool[j]);
        }

        uint32_t lhs_mask = 0;
        for (uint8_t i = 0; i < k; ++i) {
            lhs_mask |= (1u << pool[i]);
        }

        pop.insert(Individual{lhs_mask, rhs, 0.0, 0.0});
    }
    return pop;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Select(
        std::unordered_set<Individual, IndividualHash> const& pop, std::mt19937& rng) const {
    std::unordered_set<Individual, IndividualHash> selected;
    selected.reserve(pop.size());

    std::uniform_real_distribution<double> dist01(0.0, 1.0);

    Individual const* best = nullptr;
    double best_confidence = -1.0;

    for (auto const& ind : pop) {
        if (ind.confidence > best_confidence) {
            best_confidence = ind.confidence;
            best = &ind;
        }

        if (dist01(rng) < Fitness(ind.confidence)) {
            selected.emplace(ind);
        }
    }

    if (selected.empty()) {
        selected.emplace(*best);
    }

    return selected;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Crossover(
        std::unordered_set<Individual, IndividualHash> const& selected, std::mt19937& rng) const {
    std::unordered_set<Individual, IndividualHash> offspring;
    size_t const n = selected.size();
    if (n < 2) return offspring;

    offspring.reserve(std::min(n * (n - 1), static_cast<size_t>(population_size_ + 200)));

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
                int cnt = (diff_cnt > 0) ? std::uniform_int_distribution<int>(1, diff_cnt)(rng) : 0;
                while (cnt--) {
                    uint32_t bit = diff & -diff;
                    mask1 ^= bit;
                    mask2 ^= bit;
                    diff &= diff - 1;
                }
            }

            if (rhs1 != rhs2 && coin(rng)) {
                std::swap(rhs1, rhs2);
            }

            if ((mask1 != 0) && !(mask1 & (1u << rhs1))) offspring.emplace(mask1, rhs1, 0.0, 0.0);
            if ((mask2 != 0) && !(mask2 & (1u << rhs2))) offspring.emplace(mask2, rhs2, 0.0, 0.0);
        }
    }
    return offspring;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Mutate(
        std::unordered_set<Individual, IndividualHash> const& pop, std::mt19937& rng) const {
    std::unordered_set<Individual, IndividualHash> mutated;
    mutated.reserve(pop.size());

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
                uint32_t avail = (full_mask_) & ~mask & ~(1u << rhs);
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
                uint32_t avail = (full_mask_) & ~mask;
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
            mutated.emplace(mask, rhs, 0.0, 0.0);
        else
            mutated.insert(ind);
    }
    return mutated;
}

std::unordered_set<RFD, RFDHash> GaRfd::Finalize(
        std::unordered_set<Individual, IndividualHash> const& pop) const {
    std::unordered_set<RFD, RFDHash> res;
    res.reserve(pop.size());

    for (auto const& ind : pop) {
        if (ind.confidence < eps_) continue;
        res.emplace(ind.lhs_mask, ind.rhs_index, ind.support, ind.confidence);
    }
    LOG_INFO("Finalized {} unique RFDs", res.size());
    return res;
}

unsigned long long GaRfd::ExecuteInternal() {
    return ::util::TimedInvoke([&]() {
        LOG_INFO("Build similarity bitsets...");
        BuildSimilarityBitsets();

        std::mt19937 rng(seed_);

        auto pop = InitializePopulation(rng);
        EvaluatePopulation(pop);

        for (size_t gen = 0; gen < max_generations_; gen++) {
            LOG_INFO("Generation {}/{} (pop size: {})", gen + 1, max_generations_, pop.size());
            if (pop.size() >= population_size_ && AllOf(pop)) {
                LOG_INFO("All individuals satisfy confidence threshold - stopping early");
                break;
            }

            if (pop.empty()) [[unlikely]] {
                LOG_INFO("Population is empty, stopping evolution");
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
