#pragma once

#include <boost/functional/hash.hpp>

#include "cfd/model/cfd_relation_data.h"
#include "cfd/model/cfd_types.h"

namespace algos::cfd_verifier {
class CFDStatsCalculator {
private:
    std::shared_ptr<cfd::CFDRelationData> relation_;

    cfd::ItemsetCFD rule_;
    std::vector<int> lhs_attrs_;
    int rhs_attr_index_ = -1;

    std::vector<bool> support_mask_;
    std::unordered_map<cfd::Itemset, std::vector<int>, boost::hash<cfd::Itemset>> lhs_to_row_nums_;
    std::unordered_map<cfd::Itemset, cfd::Item, boost::hash<cfd::Itemset>> most_frequent_rhs_;
    int support_ = 0;
    double confidence_ = 0.0;
    std::vector<size_t> violating_rows_;
    std::vector<size_t> satisfying_rows_;

    void CreateSupportMask();
    void MakeLhsToRowNums();
    void DetermineMostFrequentRHS();
    void CalculateSupportAndConfidence();

public:
    CFDStatsCalculator(std::shared_ptr<cfd::CFDRelationData> relation, cfd::ItemsetCFD rule)
        : relation_(std::move(relation)), rule_(std::move(rule)) {
        lhs_attrs_ = relation_->GetAttrVector(rule_.first);
        rhs_attr_index_ = relation_->GetAttrIndex(rule_.second);
    };

    CFDStatsCalculator() = default;

    void CalculateStatistics();

    void ResetState();

    size_t GetNumRowsViolatingCFD() const {
        return violating_rows_.size();
    }

    std::vector<size_t> GetRowsViolatingCFD() const {
        return violating_rows_;
    };

    std::vector<size_t> GetRowsSatisfyingCFD() const {
        return satisfying_rows_;
    }

    int GetSupport() const {
        return support_;
    }

    double GetConfidence() const {
        return confidence_;
    }
};

}  // namespace algos::cfd_verifier
