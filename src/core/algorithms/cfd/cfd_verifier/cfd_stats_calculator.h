#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>

#include "cfd/model/cfd_relation_data.h"
#include "cfd/model/cfd_types.h"
#include "highlight.h"

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
    std::vector<cfd_verifier::Highlight> highlights_;
    size_t num_rows_violating_cfd_ = 0;

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

    size_t GetNumClustersViolatingCFD() const {
        return highlights_.size();
    }

    std::vector<Highlight> const& GetHighlights() const {
        return highlights_;
    }

    size_t GetNumRowsViolatingCFD() const {
        return num_rows_violating_cfd_;
    }

    int GetSupport() const {
        return support_;
    }

    double GetConfidence() const {
        return confidence_;
    }
};

}  // namespace algos::cfd_verifier
