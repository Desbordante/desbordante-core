#include "ga_rfd.h"

#include <algorithm>
#include <bit>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <numeric>
#include <ranges>
#include <string>
#include <unordered_set>

#include "core/algorithms/rfd/distance_metric.h"
#include "core/config/custom_metric/custom_metrics/option.h"
#include "core/config/descriptions.h"
#include "core/config/exceptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/custom_metric/custom_metric.h"
#include "core/util/logger.h"

namespace {

template <typename T>
inline bool InRangeInclusive(T value, T min, T max) noexcept {
    return min <= value && value <= max;
}

std::string BitRepresentation(uint32_t mask, int num_bits = 31) {
    return std::bitset<32>(mask).to_string().substr(32 - num_bits);
}

inline int FirstSetBitIndex(uint32_t value) noexcept {
    return value == 0 ? -1 : static_cast<int>(std::countr_zero(value));
}

}  // namespace

namespace algos::rfd {

GaRfd::GaRfd() : Algorithm() {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName()});
}

void GaRfd::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kRfdMinSimilarity, kRfdMinimumConfidence, kPopulationSize,
                          kRfdMaxGenerations, kRfdCrossoverProbability, kRfdMutationProbability,
                          kSeed, kMetrics, kCacheMaxSize});
}

void GaRfd::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto get_num_columns = [this]() { return input_table_->GetNumberOfColumns(); };

    auto check_probability_range = [](double value) {
        if (!InRangeInclusive(value, 0.0, 1.0))
            throw config::ConfigurationError("Probability-like option must be in [0, 1]");
    };
    auto check_population_size = [](std::size_t population_size) {
        if (population_size == 0)
            throw config::ConfigurationError("population_size must be positive");
    };

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::MetricsOption{kMetrics, kDRfdMetrics}(&metrics_, get_num_columns));
    RegisterOption(
            Option<std::vector<double>>{
                    &min_similarity_, kRfdMinSimilarity, kDRfdMinSimilarity,
                    Option<std::vector<double>>::DefaultFunc([get_num_columns]() {
                        return std::vector<double>(get_num_columns(), 1.0);
                    })}
                    .SetNormalizeFunc([get_num_columns](auto& similarities) {
                        if (similarities.size() == 1) {
                            similarities.assign(get_num_columns(), similarities.front());
                        }
                    })
                    .SetValueCheck([get_num_columns](std::vector<double> const& similarities) {
                        if (!std::ranges::all_of(similarities, [](double value) {
                                return InRangeInclusive(value, 0.0, 1.0);
                            })) {
                            throw config::ConfigurationError(
                                    "min_similarity values must be in [0, 1]");
                        }
                        if (similarities.size() != get_num_columns()) {
                            throw config::ConfigurationError(
                                    "min_similarity size must be 1 or match the number of "
                                    "attributes");
                        }
                    }));
    RegisterOption(Option{&min_confidence_, kRfdMinimumConfidence, kDRfdMinimumConfidence, 1.0}
                           .SetValueCheck(check_probability_range));
    RegisterOption(Option{&population_size_, kPopulationSize, kDPopulationSize,
                          static_cast<std::size_t>(1024)}
                           .SetValueCheck(check_population_size));
    RegisterOption(Option{&max_generations_, kRfdMaxGenerations, kDRfdMaxGenerations,
                          static_cast<std::size_t>(32)});
    RegisterOption(Option{&crossover_probability_, kRfdCrossoverProbability,
                          kDRfdCrossoverProbability, 1.0}
                           .SetValueCheck(check_probability_range));
    RegisterOption(
            Option{&mutation_probability_, kRfdMutationProbability, kDRfdMutationProbability, 1.0}
                    .SetValueCheck(check_probability_range));
    RegisterOption(Option{&seed_, kSeed, kDSeed, static_cast<std::uint32_t>(123)});
    RegisterOption(Option{&cache_max_size_, kCacheMaxSize, kDCacheMaxSize,
                          static_cast<std::size_t>(10000)});
}

void GaRfd::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, true);
    input_table_->Reset();

    num_attributes_ = typed_relation_->GetNumColumns();
    num_rows_ = typed_relation_->GetNumRows();
    if (num_attributes_ < 2)
        throw config::ConfigurationError("GA-RFD requires at least 2 attributes");
    if (num_attributes_ > kMaxAttributes)
        throw config::ConfigurationError("GA-RFD supports at most 31 attributes");
    if (num_rows_ < 2) throw config::ConfigurationError("GA-RFD requires at least 2 rows");
    full_mask_ = (1u << num_attributes_) - 1;

    total_pairs_ = num_rows_ * (num_rows_ - 1) / 2;

    for (auto& metric : metrics_) {
        if (metric == nullptr ||
            dynamic_cast<::util::DefaultCustomMetric const*>(metric.get()) != nullptr) {
            metric = EqualityMetric();
        }
    }

    LOG_INFO("Loaded {} rows, {} attributes, {} total pairs", num_rows_, num_attributes_,
             total_pairs_);
}

void GaRfd::BuildMatchBitsets() {
    if (!support_cache_) {
        support_cache_ = std::make_unique<util::LRUCache<uint32_t, std::size_t>>(cache_max_size_);
    }
    std::size_t const num_words_per_attribute = (total_pairs_ + 63) / 64;
    attribute_match_bits_.assign(num_attributes_,
                                 std::vector<uint64_t>(num_words_per_attribute, 0));

    auto const& column_data = typed_relation_->GetColumnData();
    for (std::size_t attribute = 0; attribute < num_attributes_; ++attribute) {
        auto const& column = column_data[attribute];
        auto& bits = attribute_match_bits_[attribute];
        // Core uses distances internally; the user-facing threshold is a
        // similarity in [0, 1], so distance threshold is 1 - similarity
        double const max_distance = 1.0 - min_similarity_[attribute];
        auto const& metric = *metrics_[attribute];
        model::Type const& column_type = column.GetType();

        // NULLs and empty values never count as matching
        bool const is_mixed = column.GetTypeId() == model::TypeId::kMixed;
        std::vector<bool> valid(is_mixed ? 0 : num_rows_);
        if (!is_mixed) {
            for (std::size_t row = 0; row < num_rows_; ++row) {
                valid[row] = !column.IsNullOrEmpty(row);
            }
        }

        uint64_t word_mask = 1;
        std::size_t word_index = 0;

        for (std::size_t first_row = 0; first_row < num_rows_; ++first_row) {
            std::byte const* first_value;
            if (is_mixed) {
                first_value = column.GetValue(first_row);
            } else {
                first_value = valid[first_row] ? column.GetValue(first_row) : nullptr;
            }
            for (std::size_t second_row = first_row + 1; second_row < num_rows_; ++second_row) {
                bool match = first_value != nullptr && (is_mixed || valid[second_row]) &&
                             metric.Dist(&column_type, first_value, column.GetValue(second_row)) <=
                                     max_distance;
                if (match) {
                    bits[word_index] |= word_mask;
                }
                word_mask <<= 1;
                if (word_mask == 0) {
                    word_mask = 1;
                    ++word_index;
                }
            }
        }
        LOG_INFO("Finished attribute {} match bitset", attribute);
    }
    LOG_INFO("Match bitsets built for {} attributes", num_attributes_);
}

std::size_t GaRfd::ComputeSupport(uint32_t attributes_mask) const {
    if (auto cached = support_cache_->Get(attributes_mask)) return *cached;

    if (attributes_mask == 0) [[unlikely]] {
        support_cache_->Put(0, total_pairs_);
        return total_pairs_;
    }

    uint32_t remaining_mask = attributes_mask;
    int first_attribute = FirstSetBitIndex(remaining_mask);
    if (first_attribute < 0) [[unlikely]] {
        support_cache_->Put(attributes_mask, 0);
        return 0;
    }

    auto const& first_bits = attribute_match_bits_[first_attribute];

    if (first_bits.empty()) [[unlikely]] {
        support_cache_->Put(attributes_mask, 0);
        return 0;
    }

    if ((attributes_mask & (attributes_mask - 1)) == 0u) {
        std::size_t support = 0;
        for (uint64_t word : first_bits) {
            support += std::popcount(word);
        }
        support_cache_->Put(attributes_mask, support);
        LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attributes_mask, num_attributes_),
                  support);
        return support;
    }

    std::size_t const vector_size = first_bits.size();
    if (compute_buffer_.size() != vector_size) {
        compute_buffer_.resize(vector_size);
    }

    std::memcpy(compute_buffer_.data(), first_bits.data(), vector_size * sizeof(uint64_t));

    remaining_mask &= remaining_mask - 1;
    while (remaining_mask != 0) {
        int attribute = FirstSetBitIndex(remaining_mask);
        auto const& other_bits = attribute_match_bits_[attribute];
        std::size_t running_support = 0;
        for (std::size_t word = 0; word < vector_size; ++word) {
            compute_buffer_[word] &= other_bits[word];
            running_support += std::popcount(compute_buffer_[word]);
        }
        if (running_support == 0) [[unlikely]] {
            support_cache_->Put(attributes_mask, 0);
            return 0;
        }
        remaining_mask &= remaining_mask - 1;
    }

    std::size_t support = 0;
    for (std::size_t word = 0; word < vector_size; ++word) {
        support += std::popcount(compute_buffer_[word]);
    }

    support_cache_->Put(attributes_mask, support);
    LOG_DEBUG("Support for mask {} = {}", BitRepresentation(attributes_mask, num_attributes_),
              support);
    return support;
}

GaRfd::Individual GaRfd::Evaluate(Individual const& individual) const {
    uint32_t const lhs_mask = individual.lhs_mask;
    uint8_t const rhs_index = individual.rhs_index;

    double support_lhs = static_cast<double>(ComputeSupport(lhs_mask)) / total_pairs_;
    if (support_lhs == 0.0) [[unlikely]] {
        return {lhs_mask, rhs_index, 0.0, 0.0};
    }

    uint32_t const both_mask = lhs_mask | (1u << rhs_index);
    double support_both = static_cast<double>(ComputeSupport(both_mask)) / total_pairs_;
    double confidence = support_both / support_lhs;
    return {lhs_mask, rhs_index, confidence, support_lhs};
}

void GaRfd::EvaluatePopulation(std::unordered_set<Individual, IndividualHash>& population) const {
    auto it = population.begin();
    while (it != population.end()) {
        auto node = population.extract(it++);
        if (!node.empty()) {
            node.value() = Evaluate(node.value());
            population.insert(std::move(node));
        }
    }
}

bool GaRfd::AllConfidencesAboveThreshold(
        std::unordered_set<Individual, IndividualHash> const& population) const {
    assert(!population.empty());
    return std::ranges::all_of(population, [this](Individual const& individual) {
        return individual.confidence >= min_confidence_;
    });
}

double GaRfd::Fitness(double confidence) const noexcept {
    return confidence >= min_confidence_ ? 1.0 : confidence / min_confidence_;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::InitializePopulation(
        std::mt19937& random_generator) const {
    std::unordered_set<Individual, IndividualHash> population;
    population.reserve(population_size_);

    std::uniform_int_distribution<uint8_t> rhs_dist(0, num_attributes_ - 1);
    std::uniform_int_distribution<uint8_t> lhs_size_dist(1, num_attributes_ - 1);
    std::uniform_int_distribution<uint8_t> shuffle_dist;

    std::vector<uint8_t> all_indices(num_attributes_);
    std::iota(all_indices.begin(), all_indices.end(), 0);

    std::vector<uint8_t> pool(num_attributes_);
    uint8_t const last_index = static_cast<uint8_t>(num_attributes_ - 1);

    std::size_t attempts = 0;
    while (population.size() < population_size_ && attempts++ < population_size_ * 2 + 1) {
        uint8_t const rhs_index = rhs_dist(random_generator);
        uint8_t const lhs_size = lhs_size_dist(random_generator);

        std::memcpy(pool.data(), all_indices.data(), num_attributes_ * sizeof(uint8_t));

        std::swap(pool[rhs_index], pool[last_index]);

        for (uint8_t position = 0; position < lhs_size; ++position) {
            using ParamType = std::uniform_int_distribution<uint8_t>::param_type;
            uint8_t swap_position =
                    shuffle_dist(random_generator, ParamType(position, last_index - 1));
            std::swap(pool[position], pool[swap_position]);
        }

        uint32_t lhs_mask = 0;
        for (uint8_t position = 0; position < lhs_size; ++position) {
            lhs_mask |= (1u << pool[position]);
        }

        population.insert(Individual{lhs_mask, rhs_index, 0.0, 0.0});
    }
    return population;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Select(
        std::unordered_set<Individual, IndividualHash> const& population,
        std::mt19937& random_generator) const {
    std::unordered_set<Individual, IndividualHash> selected;
    selected.reserve(population.size());

    std::uniform_real_distribution<double> uniform01(0.0, 1.0);

    Individual const* best_individual = nullptr;
    double best_confidence = -1.0;

    for (auto const& individual : population) {
        if (individual.confidence > best_confidence) {
            best_confidence = individual.confidence;
            best_individual = &individual;
        }

        if (uniform01(random_generator) < Fitness(individual.confidence)) {
            selected.emplace(individual);
        }
    }

    if (selected.empty()) {
        selected.emplace(*best_individual);
    }

    return selected;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Crossover(
        std::unordered_set<Individual, IndividualHash> const& selected,
        std::mt19937& random_generator) const {
    std::unordered_set<Individual, IndividualHash> offspring;
    std::size_t const selected_size = selected.size();
    if (selected_size < 2) return offspring;

    offspring.reserve(std::min(selected_size * (selected_size - 1),
                               static_cast<std::size_t>(population_size_ + 200)));

    std::uniform_real_distribution<double> uniform01(0.0, 1.0);
    std::bernoulli_distribution coin(0.5);

    for (auto first = selected.begin(); first != selected.end(); ++first) {
        auto second = first;
        ++second;
        for (; second != selected.end(); ++second) {
            if (uniform01(random_generator) >= crossover_probability_) continue;

            Individual const& parent_first = *first;
            Individual const& parent_second = *second;

            uint32_t mask_first = parent_first.lhs_mask;
            uint32_t mask_second = parent_second.lhs_mask;
            uint8_t rhs_first = parent_first.rhs_index;
            uint8_t rhs_second = parent_second.rhs_index;

            uint32_t differing_bits = mask_first ^ mask_second;
            if (differing_bits != 0) {
                int differing_count = std::popcount(differing_bits);
                int swaps_count = differing_count > 0
                                          ? std::uniform_int_distribution<int>(
                                                    1, differing_count)(random_generator)
                                          : 0;
                while (swaps_count-- > 0) {
                    uint32_t bit = differing_bits & (~differing_bits + 1);
                    mask_first ^= bit;
                    mask_second ^= bit;
                    differing_bits &= differing_bits - 1;
                }
            }

            if (rhs_first != rhs_second && coin(random_generator)) std::swap(rhs_first, rhs_second);

            if (mask_first != 0 && (mask_first & (1u << rhs_first)) == 0)
                offspring.emplace(mask_first, rhs_first, 0.0, 0.0);
            if (mask_second != 0 && (mask_second & (1u << rhs_second)) == 0)
                offspring.emplace(mask_second, rhs_second, 0.0, 0.0);
        }
    }
    return offspring;
}

std::unordered_set<GaRfd::Individual, GaRfd::IndividualHash> GaRfd::Mutate(
        std::unordered_set<Individual, IndividualHash> const& population,
        std::mt19937& random_generator) const {
    std::unordered_set<Individual, IndividualHash> mutated;
    mutated.reserve(population.size());

    std::uniform_real_distribution<double> uniform01(0.0, 1.0);
    std::uniform_int_distribution<uint8_t> mutation_kind_dist(0, 2);

    for (auto const& individual : population) {
        if (uniform01(random_generator) >= mutation_probability_) {
            mutated.insert(individual);
            continue;
        }

        uint32_t mask = individual.lhs_mask;
        uint8_t rhs_index = individual.rhs_index;

        bool mutated_flag = false;
        switch (mutation_kind_dist(random_generator)) {
            case 0: {  // Remove one random set bit from the mask
                int set_bits = std::popcount(mask);
                if (set_bits == 0) break;
                int skip = std::uniform_int_distribution<int>(0, set_bits - 1)(random_generator);
                uint32_t remainder = mask;
                while (skip-- > 0) {
                    remainder &= remainder - 1;
                }
                mask ^= (remainder & (~remainder + 1));
                mutated_flag = true;
                break;
            }
            case 1: {  // Add a random available bit to the lhs (excludes bits already in lhs and
                       // rhs)
                uint32_t available = full_mask_ & ~mask & ~(1u << rhs_index);
                if (available == 0) break;
                int available_count = std::popcount(available);
                int skip = std::uniform_int_distribution<int>(
                        0, available_count - 1)(random_generator);
                uint32_t remainder = available;
                while (skip-- > 0) {
                    remainder &= remainder - 1;
                }
                mask |= (remainder & (~remainder + 1));
                mutated_flag = true;
                break;
            }
            case 2: {  // Move the rhs variable to a new available bit (not in lhs and curr rhs)
                uint32_t available = full_mask_ & ~mask & ~(1u << rhs_index);
                if (available == 0) break;
                int available_count = std::popcount(available);
                int skip = std::uniform_int_distribution<int>(
                        0, available_count - 1)(random_generator);
                uint32_t remainder = available;
                while (skip-- > 0) {
                    remainder &= remainder - 1;
                }
                rhs_index = static_cast<uint8_t>(std::countr_zero(remainder & (~remainder + 1)));
                mutated_flag = true;
                break;
            }
            default: {
                break;
            }
        }
        if (mutated_flag && mask != 0 && (mask & (1u << rhs_index)) == 0)
            mutated.emplace(mask, rhs_index, 0.0, 0.0);
        else
            mutated.insert(individual);
    }
    return mutated;
}

std::unordered_set<RFD, RFDHash> GaRfd::Finalize(
        std::unordered_set<Individual, IndividualHash> const& population) const {
    std::unordered_set<RFD, RFDHash> result;
    result.reserve(population.size());

    for (auto const& individual : population) {
        if (individual.confidence < min_confidence_) continue;
        result.emplace(individual.lhs_mask, individual.rhs_index, individual.support,
                       individual.confidence);
    }
    LOG_INFO("Finalized {} unique RFDs", result.size());
    return result;
}

void GaRfd::ExecuteInternal() {
    LOG_INFO("Build match bitsets...");
    BuildMatchBitsets();
    std::mt19937 random_generator(seed_);
    auto population = InitializePopulation(random_generator);
    EvaluatePopulation(population);
    for (std::size_t generation = 0; generation < max_generations_; ++generation) {
        LOG_INFO("Generation {}/{} (pop size: {})", generation + 1, max_generations_,
                 population.size());
        if (population.size() >= population_size_ && AllConfidencesAboveThreshold(population)) {
            LOG_INFO("All individuals satisfy confidence threshold - stopping early");
            break;
        }
        if (population.empty()) [[unlikely]] {
            LOG_INFO("Population is empty, stopping evolution");
            break;
        }
        auto selected = Select(population, random_generator);
        auto offspring = Crossover(selected, random_generator);
        auto mutated = Mutate(selected, random_generator);
        population = std::move(selected);
        population.insert(offspring.begin(), offspring.end());
        population.insert(mutated.begin(), mutated.end());
        EvaluatePopulation(population);
        if (population.size() > population_size_ + 100) {
            std::vector<Individual> sorted(population.begin(), population.end());
            std::sort(sorted.begin(), sorted.end(), [](auto const& left, auto const& right) {
                return left.confidence > right.confidence;
            });
            sorted.resize(population_size_ + 100);
            population =
                    std::unordered_set<Individual, IndividualHash>(sorted.begin(), sorted.end());
        }
    }
    discovered_ = Finalize(population);
}

void GaRfd::ResetState() {
    discovered_.clear();
    support_cache_.reset();
}

GaRfd::~GaRfd() {
    ResetState();
}

}  // namespace algos::rfd
