#include "core/algorithms/rfd/ga_rfd/ga_rfd.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ranges>
#include <stdexcept>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <bitset>
#include <string>

#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/logger.h"
#include "core/util/timed_invoke.h"

namespace {

template <typename T>
[[nodiscard]] inline bool InRangeInclusive(T value, T min, T max) {
    return min <= value && value <= max;
}

[[nodiscard]] std::string bit_representation(uint32_t mask, int num_bits = 31) {
    return std::bitset<32>(mask).to_string().substr(32 - num_bits);
}

[[nodiscard]] inline int first_set_bit(uint32_t x) {
    return x ? static_cast<int>(__builtin_ctz(x)) : -1;
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
    num_rows_ = 0;
    column_data_.clear();

    if (!input_table_->HasNextRow()) [[unlikely]]
        throw std::runtime_error("Input table is empty");

    auto first_row = input_table_->GetNextRow();
    num_attrs_ = first_row.size();
    if (num_attrs_ < 2) [[unlikely]]
        throw std::runtime_error("GA-rfd requires at least 2 attributes");
    if (num_attrs_ > 31) [[unlikely]]
        throw std::runtime_error("Maximum 31 attributes supported");
    column_data_.resize(num_attrs_);
    for (size_t i = 0; i < num_attrs_; ++i) {
        column_data_[i].push_back(std::move(first_row[i]));
    }
    num_rows_ = 1;

    while (input_table_->HasNextRow()) {
        auto row = input_table_->GetNextRow();
        if (row.size() != num_attrs_)
            throw std::runtime_error("Inconsistent number of attributes in row");
        for (size_t i = 0; i < num_attrs_; ++i) {
            column_data_[i].push_back(std::move(row[i]));
        }
        num_rows_++;
    }

    if (num_rows_ < 2) [[unlikely]] 
        throw std::runtime_error("Input table must contain at least 2 rows");

    if (num_rows_ > std::numeric_limits<std::size_t>::max() / (num_rows_ - 1) / 2)
        throw std::runtime_error("Table too large, total pairs would overflow size_t");
    total_pairs_ = num_rows_ * (num_rows_ - 1) / 2;

    LOG_INFO("Loaded {} rows, {} attributes, {} total pairs", num_rows_, num_attrs_, total_pairs_);

    if (metrics_.empty()) {
        metrics_.clear();
        metrics_.reserve(num_attrs_);
        for (size_t i = 0; i < num_attrs_; ++i)
            metrics_.emplace_back(EqualityMetric());
    }
    if (metrics_.size() != num_attrs_) [[unlikely]]
        throw std::invalid_argument("The number of attributes and metrics do not match");
}

void GaRfd::BuildSimilarityBitsets() {
    LOG_INFO("BuildSimilarityBitsets: total_pairs_ = {}, num_attrs_ = {}, num_rows_ = {}",
              total_pairs_, num_attrs_, num_rows_);
    const std::size_t num_uint64_per_attr = (total_pairs_ + 63) / 64;
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
                    const double sim = metrics_[a]->Compare(col[i], col[j]);
                    if (sim >= min_similarity_) {
                        const std::size_t word_idx = pair_idx / 64;
                        const std::size_t bit_idx = pair_idx % 64;
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
        LOG_INFO("Finished attribute {} similarity bitset", a);
    }
    LOG_INFO("Similarity bitsets built for {} attributes", num_attrs_);
}

std::size_t GaRfd::ComputeSupport(uint32_t attrs_mask) const {
    if (auto cached = support_cache_.get(attrs_mask)) return *cached;
    if (attrs_mask == 0) {
        support_cache_.put(0, total_pairs_);
        return total_pairs_;
    }

    uint32_t mm = attrs_mask;
    int first = first_set_bit(mm);
    if (first < 0) { support_cache_.put(attrs_mask, 0); return 0; }

    const auto &first_vec = attr_similarity_bits_[first];
    if ((attrs_mask & (attrs_mask - 1)) == 0u) {
        std::size_t s = 0;
        for (uint64_t w : first_vec) s += std::popcount(w);
        support_cache_.put(attrs_mask, s);
        LOG_INFO("Support for mask {} = {}", bit_representation(attrs_mask, num_attrs_), s);
        return s;
    }

    std::vector<uint64_t> buffer = first_vec;
    mm &= mm - 1;
    while (mm) {
        int a = first_set_bit(mm);
        const auto &other = attr_similarity_bits_[a];
        const std::size_t N = buffer.size();
        std::size_t running = 0;
        for (std::size_t k = 0; k < N; ++k) {
            buffer[k] &= other[k];
            running += std::popcount(buffer[k]);
        }
        if (running == 0) { support_cache_.put(attrs_mask, 0); return 0; }
        mm &= mm - 1;
    }

    std::size_t support = 0;
    for (uint64_t w : buffer) support += std::popcount(w);
    support_cache_.put(attrs_mask, support);
    LOG_INFO("Support for mask {} = {}", bit_representation(attrs_mask, num_attrs_), support);
    return support;
}

GaRfd::Individual GaRfd::Evaluate(const Individual& ind) const {
    const uint32_t lhs_mask = ind.lhs_mask;
    const uint8_t rhs = ind.rhs_index;

    double support_lhs = static_cast<double>(ComputeSupport(lhs_mask)) / total_pairs_;
    if (support_lhs == 0.0) [[unlikely]] {
        return {lhs_mask, rhs, 0.0, 0.0};
    }

    const uint32_t both_mask = lhs_mask | (1u << rhs);
    double support_both = static_cast<double>(ComputeSupport(both_mask)) / total_pairs_;
    double confidence = support_both / support_lhs;
    return {lhs_mask, rhs, confidence, support_both};
}

void GaRfd::EvaluatePopulation(std::vector<Individual>& pop) const {
    for (auto& ind : pop) ind = Evaluate(ind);
}

bool GaRfd::AllOf(const std::vector<Individual>& pop) const {
    if (pop.empty()) [[unlikely]] return false;
    return std::ranges::all_of(pop, [&](Individual const& ind) { return ind.confidence >= beta_; });
}

double GaRfd::Fitness(double confidence) const noexcept {
    return (confidence >= beta_) ? 1.0 : confidence / beta_;
}

std::vector<GaRfd::Individual> GaRfd::InitializePopulation(std::mt19937& rng) const {
    std::vector<Individual> pop;
    pop.reserve(population_size_);
    std::uniform_int_distribution<uint8_t> rhs_dist(0u, static_cast<uint8_t>(num_attrs_ - 1u));

    std::vector<uint8_t> indices(num_attrs_);
    std::iota(indices.begin(), indices.end(), 0);

    while (pop.size() < population_size_) {
        uint8_t rhs = rhs_dist(rng);
        std::uniform_int_distribution<int> kdist(1, static_cast<int>(num_attrs_ - 1));
        int k = kdist(rng);
        std::vector<uint8_t> pool;
        pool.reserve(num_attrs_-1);
        for (uint8_t i = 0; i < num_attrs_; ++i) if (i != rhs) pool.push_back(i);
        for (int i = 0; i < k; ++i) {
            std::uniform_int_distribution<int> d(i, static_cast<int>(pool.size()) - 1);
            int j = d(rng);
            std::swap(pool[i], pool[j]);
        }
        uint32_t lhs_mask = 0;
        for (int i = 0; i < k; ++i) lhs_mask |= (1u << pool[i]);

        pop.emplace_back(Individual{lhs_mask, rhs, 0.0, 0.0});
    }
    return pop;
}

std::vector<GaRfd::Individual> GaRfd::Select(std::vector<Individual> const& pop,
                                             std::mt19937& rng) const {
    std::vector<Individual> selected;
    selected.reserve(pop.size());

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    for (auto const& ind : pop) {
        if (dist01(rng) < Fitness(ind.confidence)) selected.push_back(ind);
    }
    return selected;
}

std::vector<GaRfd::Individual> GaRfd::Crossover(const std::vector<Individual>& selected,
                                                std::mt19937& rng) const {
    std::vector<Individual> offspring;
    if (selected.size() < 2) return offspring;

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::bernoulli_distribution coin(0.5);

    for (std::size_t i = 0; i < selected.size(); ++i) {
        for (std::size_t j = i + 1; j < selected.size(); ++j) {
            if (dist01(rng) >= crossover_probability_) [[unlikely]] continue;

            Individual c1 = selected[i];
            Individual c2 = selected[j];

                        uint32_t mask1 = c1.lhs_mask;
            uint32_t mask2 = c2.lhs_mask;
            uint32_t diff = mask1 ^ mask2;
            auto diff_cnt = std::popcount(diff);

            if (diff_cnt > 0) {
                std::uniform_int_distribution<decltype(diff_cnt)> cnt_dist(1, diff_cnt);
                uint32_t cnt = cnt_dist(rng);
                while (cnt-- && diff) {
                    uint32_t bit = diff & -diff;
                    mask1 ^= bit;
                    mask2 ^= bit;
                    diff ^= bit;
                }
            }

            c1.lhs_mask = mask1;
            c2.lhs_mask = mask2;

            if (coin(rng)) {
                std::swap(c1.rhs_index, c2.rhs_index);
            }

            if ((c1.lhs_mask & (1u << c1.rhs_index)) || c1.lhs_mask == 0) [[unlikely]] continue;
            if ((c2.lhs_mask & (1u << c2.rhs_index)) || c2.lhs_mask == 0) [[unlikely]] continue;

            c1.confidence = 0.0;
            c1.support = 0.0;
            c2.confidence = 0.0;
            c2.support = 0.0;

            offspring.emplace_back(std::move(c1));
            offspring.emplace_back(std::move(c2));
        }
    }
    return offspring;
}

void GaRfd::Mutate(std::vector<Individual>& pop, std::mt19937& rng) const {
    if (pop.empty()) return;

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::uniform_int_distribution<uint8_t> rhs_dist(0, num_attrs_ - 1);
    std::uniform_int_distribution<uint8_t> coin(0, 2);

    for (auto& ind : pop) {
        if (dist01(rng) >= mutation_probability_) [[unlikely]] continue;

        uint32_t mask = ind.lhs_mask;
        uint8_t rhs = ind.rhs_index;

        switch (coin(rng)) {
            case 0: {
                if (mask == 0) break;
                int ones = std::popcount(mask);
                std::uniform_int_distribution<int> bit_selector(0, ones - 1);
                int skip = bit_selector(rng);
                uint32_t bit = 1;
                for (int i = 0; ; ++i) {
                    while ((mask & bit) == 0) bit <<= 1;
                    if (i == skip) break;
                    bit <<= 1;
                }
                mask ^= bit;
                break;
            }
            case 1: {
                uint32_t full_mask = ((1u << num_attrs_) - 1) & ~(1u << rhs);
                if ((mask & full_mask) == full_mask) break;
                std::uniform_int_distribution<uint8_t> new_rhs_dist(0, num_attrs_ - 1);
                uint32_t new_bit = 1u << new_rhs_dist(rng);
                while ( (mask & new_bit) || (new_bit == (1u << rhs)) ) {
                    new_bit = 1u << new_rhs_dist(rng);
                }
                mask |= new_bit;
                break;
            }
            case 2: {
                uint8_t new_rhs = rhs_dist(rng);
                while (mask & (1u << new_rhs)) new_rhs = rhs_dist(rng);
                rhs = new_rhs;
                break;
            }
        }

        if (mask == 0 || (mask & (1u << rhs))) [[unlikely]] continue;

        ind.lhs_mask = mask;
        ind.rhs_index = rhs;
        ind.confidence = 0.0;
        ind.support = 0.0;
    }
}

std::set<RFD> GaRfd::Finalize(const std::vector<Individual>& pop) const {
    std::unordered_map<uint64_t, RFD> best_rfds;
    for (auto const& ind : pop) {
        if (ind.confidence < beta_) continue;

        uint64_t key = (static_cast<uint64_t>(ind.lhs_mask) << 8) | ind.rhs_index;
        auto it = best_rfds.find(key);
        if (it == best_rfds.end()) {
            RFD rfd;
            rfd.lhs_mask = ind.lhs_mask;
            rfd.rhs_index = ind.rhs_index;
            rfd.support = ind.support;
            rfd.confidence = ind.confidence;
            best_rfds[key] = std::move(rfd);
        } else if (ind.confidence > it->second.confidence) {
            it->second.confidence = ind.confidence;
            it->second.support = ind.support;
        }
    }

    std::set<RFD> res;
    for (auto& pair : best_rfds) res.insert(std::move(pair.second));
    LOG_DEBUG("Finalized {} unique RFDs", res.size());
    return res;
}

unsigned long long GaRfd::ExecuteInternal() {
    return util::TimedInvoke([&]() {
        LOG_INFO("Build similarity bitsets...");
        BuildSimilarityBitsets();

        std::mt19937 rng(seed_);

        std::vector<Individual> pop = InitializePopulation(rng);
        EvaluatePopulation(pop);

        for (std::size_t gen = 0; gen < max_generations_; gen++) {
            LOG_INFO("Generation {}/{} (pop size: {})", gen + 1, max_generations_, pop.size());
            if (AllOf(pop)) {
                LOG_DEBUG("All individuals satisfy confidence threshold – stopping early");
                break;
            }

            std::vector<Individual> selected = Select(pop, rng);
            if (selected.empty()) [[unlikely]] {
                auto best = *std::max_element(pop.begin(), pop.end(),
                    [](auto& a, auto& b) { return a.confidence < b.confidence; });
                selected.push_back(best);
            }
            std::vector<Individual> offspring = Crossover(selected, rng);
            Mutate(offspring, rng);

            pop.clear();
            pop.reserve(selected.size() + offspring.size());
            pop.insert(pop.end(), std::make_move_iterator(selected.begin()),
                      std::make_move_iterator(selected.end()));
            pop.insert(pop.end(), std::make_move_iterator(offspring.begin()),
                      std::make_move_iterator(offspring.end()));

            if (pop.size() > 2*population_size_) {
                std::sort(pop.begin(), pop.end(),
                    [](auto const& a, auto const& b) { return a.confidence > b.confidence; });
                pop.resize(2*population_size_);
            }

            EvaluatePopulation(pop);
        }

        discovered_ = Finalize(pop);
    });
}

}  // namespace algos::rfd
