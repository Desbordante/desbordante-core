# pragma once

#include <vector>
#include <map>

#include "data/DataFrame.h"
#include "data/SchemaValue.h"
#include "predicates/SingleAttributePredicate.h"
#include "cache_with_limit.h"
#include "attribute_set.h"

namespace algos::fastod {

class StrippedPartition {
private:
    std::vector<int> indexes_;
    std::vector<int> begins_;
    const DataFrame& data_;

    static long merge_time_;
    static long validate_time_;
    static long clone_time_;
    static CacheWithLimit<AttributeSet, StrippedPartition> cache_;
    
public:
    StrippedPartition();
    StrippedPartition(const DataFrame& data) noexcept;
    StrippedPartition(const StrippedPartition& origin) noexcept;

    StrippedPartition Product(int attribute) noexcept;

    bool Split(int right) noexcept;
    bool Swap(const SingleAttributePredicate& left, int right) noexcept;

    std::string ToString() const noexcept;
    StrippedPartition DeepClone() const noexcept;
    static StrippedPartition GetStrippedPartition(const AttributeSet& attribute_set, const DataFrame& data) noexcept;

    long SplitRemoveCount(int right) noexcept;
    long SwapRemoveCount(const SingleAttributePredicate& left, int right) noexcept;

    StrippedPartition& operator=(const StrippedPartition& other);
};

} // namespace algos::fastod
