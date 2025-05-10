#pragma once

#include <sstream>
#include <string>
#include <utility>

namespace model::mde {
class RecordMatch {
    std::string left_partitioning_function_;
    std::string right_partitioning_function_;
    std::string comparison_function_;
    std::string order_;

public:
    RecordMatch(std::string left_partitioning_function, std::string right_partitioning_function,
                std::string comparison_function, std::string order)
        : left_partitioning_function_(std::move(left_partitioning_function)),
          right_partitioning_function_(std::move(right_partitioning_function)),
          comparison_function_(std::move(comparison_function)),
          order_(std::move(order)) {}

    std::string const& GetLeftPartitioningFunction() const noexcept {
        return left_partitioning_function_;
    }

    std::string const& GetRightPartitioningFunction() const noexcept {
        return right_partitioning_function_;
    }

    std::string const& GetComparisonFunction() const noexcept {
        return comparison_function_;
    }

    std::string const& GetOrder() const noexcept {
        return order_;
    }

    std::tuple<std::string, std::string, std::string, std::string> ToTuple() const {
        return {left_partitioning_function_, right_partitioning_function_, comparison_function_,
                order_};
    }

    std::string ToString() const {
        std::stringstream ss;
        ss << "(" << left_partitioning_function_ << ", " << right_partitioning_function_ << ", "
           << comparison_function_ << ", " << order_ << ")";
        return ss.str();
    }
};
}  // namespace model::mde
