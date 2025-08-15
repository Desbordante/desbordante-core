#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "algorithms/mde/decision_boundaries/decision_boundary.h"
#include "algorithms/mde/record_match.h"

namespace model::mde {
class RecordClassifier {
    RecordMatch record_match_;
    std::shared_ptr<decision_boundaries::DecisionBoundary> decision_boundary_;

public:
    RecordMatch const& GetRecordMatch() const noexcept {
        return record_match_;
    }

    decision_boundaries::DecisionBoundary const& GetDecisionBoundary() const noexcept {
        return *decision_boundary_;
    }

    std::string ToString() const {
        return decision_boundary_->ToString() + " " + record_match_.GetOrder() + " " +
               record_match_.GetComparisonFunction() + "(" +
               record_match_.GetLeftPartitioningFunction() + ", " +
               record_match_.GetRightPartitioningFunction() + ")";
    }

    std::tuple<RecordMatch, std::shared_ptr<decision_boundaries::DecisionBoundary>> ToTuple()
            const {
        return {record_match_, decision_boundary_};
    }

    RecordClassifier(RecordMatch record_match,
                     std::shared_ptr<decision_boundaries::DecisionBoundary> decision_boundary)
        : record_match_(std::move(record_match)),
          decision_boundary_(std::move(decision_boundary)) {}
};

class MDE {
    std::string left_table_;
    std::string right_table_;
    std::vector<RecordClassifier> lhs_;
    RecordClassifier rhs_;

public:
    MDE(std::string left_table, std::string right_table, std::vector<RecordClassifier> lhs,
        RecordClassifier rhs)
        : left_table_(std::move(left_table)),
          right_table_(std::move(right_table)),
          lhs_(std::move(lhs)),
          rhs_(std::move(rhs)) {}

    std::string const& GetLeftTable() const noexcept {
        return left_table_;
    }

    std::string const& GetRightTable() const noexcept {
        return right_table_;
    }

    std::vector<RecordClassifier> const& GetLhs() const noexcept {
        return lhs_;
    }

    RecordClassifier const& GetRhs() const noexcept {
        return rhs_;
    }

    std::string ToString() const {
        std::stringstream ss;
        ss << "MDE on (" << left_table_ << ", " << right_table_ << "): ";
        ss << "[";
        if (!lhs_.empty()) {
            auto it = lhs_.begin();
            ss << it->ToString();
            while (++it != lhs_.end()) {
                ss << "; " << it->ToString();
            }
        }
        ss << "] -> " << rhs_.ToString();
        return ss.str();
    }
};
}  // namespace model::mde
