#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/aidfd/search_tree.h"
#include "algorithms/fd_algorithm.h"
#include "model/column.h"
#include "model/relational_schema.h"
#include "model/vertical.h"

namespace algos {

class Aid : public FDAlgorithm {
private:
    using Cluster = std::vector<size_t>;

    std::unique_ptr<RelationalSchema> schema_{};
    std::vector<std::vector<size_t>> tuples_;

    size_t number_of_attributes_{};
    size_t number_of_tuples_{};

    std::unordered_set<boost::dynamic_bitset<>> neg_cover_{};

    constexpr static const double growth_threshold_ = 0.01;
    constexpr static const size_t window_size_ = 10;
    constexpr static const size_t prime_ = 10619863;

    std::vector<double> prev_ratios_;
    double sum_{};

    std::vector<std::unordered_map<size_t, Cluster>> clusters_;
    std::vector<std::vector<size_t>> indices_in_clusters_;

    boost::dynamic_bitset<> constant_columns_;

    void ResetStateFd() final;

    void LoadDataFd(model::IDatasetStream &data_stream) final;
    unsigned long long ExecuteInternal() final;

    void BuildClusters();
    void CreateNegativeCover();
    void InvertNegativeCover();

    void HandleTuple(size_t tuple_num, size_t index);
    void HandleInvalidFd(const boost::dynamic_bitset<>& neg_cover_el, SearchTree& pos_cover_tree,
                         size_t rhs);
    size_t GenerateSecondClusterIndex(size_t index_in_cluster, size_t iteration_num) const;
    bool IsNegativeCoverGrowthSmall(size_t iteration_num, double curr_ratio);

    void HandleConstantColumns(boost::dynamic_bitset<>& attributes);
    void RegisterFDs(size_t rhs, const std::vector<boost::dynamic_bitset<>>& list_of_lhs);

    static boost::dynamic_bitset<> ChangeAttributesOrder(
        const boost::dynamic_bitset<>& initial_bitset, const std::vector<size_t>& new_order);
    std::vector<size_t> GetAttributesSortedByFrequency(
        const std::vector<boost::dynamic_bitset<>>& neg_cover_vector) const;

    boost::dynamic_bitset<> BuildAgreeSet(size_t t1, size_t t2);

public:
    Aid();
};

}  // namespace algos
