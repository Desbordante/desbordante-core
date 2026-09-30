#pragma once

#include <cstddef>
#include <map>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/model/cfd_types.h"

namespace algos::cfd {

class TuplePattern {
public:
    using PatternValues = std::map<AttributeIndex, Item>;
    using ColumnIndices = boost::dynamic_bitset<>;

private:
    PatternValues pattern_values_;
    ColumnIndices column_indices_;

public:
    explicit TuplePattern(std::size_t columns_number, PatternValues pattern_values = {});

    std::size_t Size() const;
    PatternValues const& GetPatternValues() const;
    Item GetPatternValue(AttributeIndex column_index) const;
    ColumnIndices const& GetColumnIndices() const;

    bool HasColumnPattern(AttributeIndex column_index, Item item) const;
    bool IsConst() const;
    bool IsVar() const;

    TuplePattern GetWithoutColumn(AttributeIndex column_index) const;
    Itemset ToItemset() const;

    bool operator==(TuplePattern const& rhs) const;
    bool operator<=(TuplePattern const& rhs) const;
    bool IsMoreGeneralThan(TuplePattern const& rhs) const;

    static TuplePattern UnionTuplePatterns(TuplePattern const& lhs, TuplePattern const& rhs);
};

}  // namespace algos::cfd
