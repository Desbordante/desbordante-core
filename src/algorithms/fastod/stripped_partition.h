#pragma once

#include <vector>
#include <map>

#include "data_frame.h"
#include "schema_value.h"
#include "single_attribute_predicate.h"
#include "cache_with_limit.h"
#include "attribute_set.h"

namespace algos::fastod {

class StrippedPartition {
private:
    std::vector<std::size_t> indexes_;
    std::vector<std::size_t> begins_;
    const DataFrame& data_;

    static long merge_time_;
    static long validate_time_;
    static long clone_time_;
    static CacheWithLimit<AttributeSet, StrippedPartition> cache_;
    
public:
    explicit StrippedPartition(const DataFrame& data) noexcept;
    StrippedPartition(const StrippedPartition& origin) noexcept;

    StrippedPartition Product(std::size_t attribute) noexcept;

    bool Split(std::size_t right) noexcept;
    bool Swap(const SingleAttributePredicate& left, std::size_t right) noexcept;

    std::string ToString() const noexcept;
    StrippedPartition DeepClone() const noexcept;
    static StrippedPartition GetStrippedPartition(const AttributeSet& attribute_set, const DataFrame& data) noexcept;

    long SplitRemoveCount(std::size_t right) noexcept;
    long SwapRemoveCount(const SingleAttributePredicate& left, std::size_t right) noexcept;

    StrippedPartition& operator=(const StrippedPartition& other);
};

} // namespace algos::fastod
