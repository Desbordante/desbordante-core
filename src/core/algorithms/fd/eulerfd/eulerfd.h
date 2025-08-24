#pragma once

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <boost/unordered_set.hpp>

#include "config/equal_nulls/option.h"
#include "config/tabular_data/input_table/option.h"
#include "custom_random_seed/option.h"
#include "custom_random_seed/type.h"
#include "fd/fd_algorithm.h"
#include "mlfq.h"
#include "model/table/column.h"
#include "model/table/relational_schema.h"
#include "model/table/vertical.h"
#include "search_tree.h"
#include "util/custom_random.h"

namespace algos {

class EulerFD : public FDAlgorithm {
    using Bitset = boost::dynamic_bitset<>;
    using RandomStrategy = Cluster::RandomStrategy;

    // Random strategy for unit tests
    config::CustomRandomSeedType custom_random_opt_;
    RandomStrategy rand_function_;
    std::unique_ptr<CustomRandom> random_{};
    constexpr static std::size_t kRandomUpperBound = 3047102;

    // Data from load data
    size_t number_of_attributes_{};
    size_t number_of_tuples_{};
    config::InputTable input_table_;
    std::shared_ptr<RelationalSchema> schema_{};
    std::vector<std::vector<size_t>> tuples_;

    config::EqNullsType is_null_equal_null_{};

    // Thresholds to checking criterion of EulerFD cycles
    constexpr static double kPosCoverGrowthThreshold = 0.01;
    constexpr static double kNegCoverGrowthThreshold = 0.01;
    constexpr static size_t kWindow = 3;
    std::array<double, kWindow> last_ncover_ratios_;
    std::array<double, kWindow> last_pcover_ratios_;

    // Clusters from partition
    std::vector<Cluster> clusters_;
    Bitset constant_columns_;

    // mlfq sampling data
    bool is_first_sample_ = true;
    constexpr static size_t const kQueuesNumber = 5;
    MLFQ mlfq_;
    constexpr static double kInitialEffectiveThreshold = 0.01;
    double effective_threshold_ = kInitialEffectiveThreshold;

    // Invalid fds storages
    std::unordered_set<Bitset> invalids_;
    std::unordered_set<Bitset> new_invalids_;
    size_t fd_num_ = 0;
    size_t old_invalid_size_ = 0;

    // Covers of fds
    std::vector<SearchTreeEulerFD> negative_cover_;
    std::vector<SearchTreeEulerFD> positive_cover_;

    // Data for sorting columns by number of 1
    std::vector<size_t> attribute_indexes_;
    std::vector<size_t> attribute_frequencies_;

    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;
    void ResetStateFd() final;
    void MakeExecuteOptsAvailable() final;

    [[nodiscard]] Bitset BuildAgreeSet(size_t t1, size_t t2) const;

    void InitCovers();
    void BuildPartition();

    double SamplingInCluster(Cluster *cluster);
    void Sampling();
    size_t GenerateResults();

    [[nodiscard]] std::vector<size_t> GetAttributesSortedByFrequency(
            std::vector<Bitset> const &neg_cover_vector);
    [[nodiscard]] static Bitset ChangeAttributesOrder(Bitset const &initial_bitset,
                                                      std::vector<size_t> const &new_order);

    [[nodiscard]] std::vector<Bitset> CreateNegativeCover(
            size_t rhs, std::vector<Bitset> const &neg_cover_vector);
    size_t Invert(size_t rhs, std::vector<Bitset> const &neg);

    static void AddInvalidAtTree(SearchTreeEulerFD &tree, Bitset const &invalid);
    static std::unordered_set<Bitset> RemoveGeneralizations(SearchTreeEulerFD &tree,
                                                            Bitset const &invalid);

    bool IsNCoverGrowthSmall() const;
    bool IsPCoverGrowthSmall() const;

    void SaveAnswer();

public:
    EulerFD();
};
}  // namespace algos
