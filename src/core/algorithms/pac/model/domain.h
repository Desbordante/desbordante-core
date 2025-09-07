#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "pac/model/metrizable_tuple.h"

/// @brief An ordered domain in metric space of values
namespace pac::model {
class Domain {
private:
    std::shared_ptr<MetrizableTupleType> type_;
    Tuple first_;
    Tuple last_;

    std::vector<std::byte> first_data_;
    std::vector<std::byte> last_data_;

public:
    /// @brief Use pointers to some data as @c first and @c last
    Domain(std::shared_ptr<MetrizableTupleType> type, Tuple&& first, Tuple&& last)
        : type_(std::move(type)), first_(first), last_(last) {}

    /// @brief Use data as @c first and @c last and hold it in this Domain object
    Domain(std::shared_ptr<MetrizableTupleType> type, std::vector<std::byte>&& first_data,
           std::vector<std::byte>&& last_data)
        : type_(std::move(type)),
          first_data_(std::move(first_data)),
          last_data_(std::move(last_data)) {
        std::transform(first_data_.begin(), first_data_.end(), std::back_inserter(first_),
                       [](std::byte const& data) { return &data; });
        std::transform(last_data_.begin(), last_data_.end(), std::back_inserter(last_),
                       [](std::byte const& data) { return &data; });
    }

    Tuple const& GetFirst() const {
        return first_;
    }

    Tuple const& GetLast() const {
        return last_;
    }

    std::string ToString() const {
        std::ostringstream oss;
        oss << '(' << type_->ValueToString(first_) << ", " << type_->ValueToString(last_) << ')';
        return oss.str();
    }
};
}  // namespace pac::model
