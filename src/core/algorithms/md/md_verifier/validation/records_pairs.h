#pragma once
#include <utility>

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/similarity_data.h"
#include "algorithms/md/similarity.h"
#include "model/index.h"

namespace algos::md {
using RecordsPair = std::pair<model::Index, model::Index>;
using RecordsSet = boost::unordered_set<model::Index>;
using RecordsPairsSet = boost::unordered_map<model::Index, RecordsSet>;
using RecordsPairToSimilarityMap = boost::unordered_map<RecordsPair, model::md::Similarity>;

class ViolatingRecordsSet {
private:
    RecordsPairsSet records_pairs_;

public:
    ViolatingRecordsSet() = default;

    ViolatingRecordsSet(RecordsPairsSet const& records_pairs) : records_pairs_(records_pairs) {}

    ViolatingRecordsSet(RecordsPairsSet&& records_pairs)
        : records_pairs_(std::move(records_pairs)) {}

    void InsertClusters(hymd::indexes::PliCluster const& left_cluster,
                        hymd::indexes::PliCluster const& right_cluster);
    void DeleteClusters(hymd::indexes::PliCluster const& left_cluster,
                        hymd::indexes::PliCluster const& right_cluster);

    void Clear();

    void Fill(std::size_t left_size, std::size_t right_size);

    bool Empty() const {
        return records_pairs_.empty();
    }

    RecordsPairsSet const& GetPairs() const {
        return records_pairs_;
    }
};

class IntersectionBuilder {
private:
    RecordsPairsSet const& original_;
    RecordsPairsSet intersection_;

public:
    IntersectionBuilder(ViolatingRecordsSet const& records) : original_(records.GetPairs()) {}

    void AddIntersection(hymd::indexes::PliCluster const& left_cluster,
                         hymd::indexes::PliCluster const& right_cluster);

    ViolatingRecordsSet Build() {
        return ViolatingRecordsSet(std::move(intersection_));
    }
};
}  // namespace algos::md
