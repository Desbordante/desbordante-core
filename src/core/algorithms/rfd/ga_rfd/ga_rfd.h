#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <set>
#include <list>
#include <optional>
#include <memory>
#include <iterator>
#include <cassert> 

#include "core/algorithms/algorithm.h"
#include "core/algorithms/rfd/rfd.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_index.h"

namespace algos::rfd {

template <typename K, typename V>
class LRUCache {
    struct Entry {
        V value;
        typename std::list<K>::iterator it;
    };
    std::unordered_map<K, Entry> map_;
    std::list<K> list_;
    std::size_t max_size_;

public:
    explicit LRUCache(std::size_t max_size) : max_size_(max_size) {
        assert(max_size > 0 && "LRUCache max_size must be positive");
    }

    std::optional<V> get(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) return std::nullopt;
        list_.splice(list_.end(), list_, it->second.it);
        return it->second.value;
    }

    void put(const K& key, const V& value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second.value = value;
            list_.splice(list_.end(), list_, it->second.it);
            return;
        }
        if (map_.size() >= max_size_) {
            K lru_key = list_.front();
            list_.pop_front();
            map_.erase(lru_key);
        }
        list_.push_back(key);
        map_[key] = {value, std::prev(list_.end())};
    }

    void clear() noexcept {
        map_.clear();
        list_.clear();
    }
};

class GaRfd : public algos::Algorithm {
private:
    // Input
    config::InputTable input_table_;
    std::vector<std::shared_ptr<SimilarityMetric>> metrics_;

    // Internal state
    std::vector<std::vector<std::string>> column_data_;
    uint8_t num_attrs_ = 0;
    std::size_t num_rows_ = 0;
    std::size_t total_pairs_ = 0;

    // separate bin column on chunk of 64 bit
    std::vector<std::vector<uint64_t>> attr_similarity_bits_;

    static constexpr std::size_t kCacheMaxSize = 10000;
    mutable LRUCache<uint32_t, std::size_t> support_cache_{kCacheMaxSize};

    // Parameters
    double min_similarity_ = 0.7;           // similarity threshold for a pair of values
    double beta_ = 0.75;                    // minimum confidence for RFD
    std::size_t max_generations_ = 50;
    std::size_t population_size_ = 20;
    double crossover_probability_ = 0.85;
    double mutation_probability_ = 0.3;
    std::uint64_t seed_ = 0;                // random number generator seed

    std::set<RFD> discovered_;

    // Algorithm overrides
    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;
    void ResetState() final { discovered_.clear(); }

    struct Individual {
        uint32_t lhs_mask = 0;
        uint8_t rhs_index = 0;
        double confidence = 0.0;
        double support = 0.0;
    };

    // helper methods
    void BuildSimilarityBitsets();
    [[nodiscard]] std::size_t ComputeSupport(uint32_t attrs_mask) const;
    // Computes conf and supp for a single individual
    [[nodiscard]] Individual Evaluate(const Individual& ind) const;
    // Computes conf and supp for all individuals
    void EvaluatePopulation(std::vector<Individual>& pop) const;
    // Checks each individual threshold satisfies conf
    [[nodiscard]] bool AllOf(const std::vector<Individual>& pop) const;
    // Computes fitness from conf: 1.0 if confidence >= beta, else confidence / beta.
    [[nodiscard]] double Fitness(double confidence) const noexcept;

    // GA methods
    [[nodiscard]] std::vector<Individual> InitializePopulation(std::mt19937_64& rng) const;
    [[nodiscard]] std::vector<Individual> Select(const std::vector<Individual>& pop,
                                                 std::mt19937_64& rng) const;
    [[nodiscard]] std::vector<Individual> Crossover(const std::vector<Individual>& selected,
                                                    std::mt19937_64& rng) const;
    void Mutate(std::vector<Individual>& pop, std::mt19937_64& rng) const;

    [[nodiscard]] std::set<RFD> Finalize(const std::vector<Individual>& pop) const;

public:
    GaRfd();

    void SetMetrics(std::vector<std::shared_ptr<SimilarityMetric>> metrics) noexcept { metrics_ = std::move(metrics); }
    [[nodiscard]] std::vector<RFD> GetRfds() const {
        return {discovered_.begin(), discovered_.end()};
    }
};

}  // namespace algos::rfd
