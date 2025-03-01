#pragma once

#include <cstddef>        // for byte
#include <unordered_map>  // for unordered_map
#include <unordered_set>  // for unordered_set
#include <utility>        // for pair
#include <vector>         // for vector

#include <boost/container_hash/hash.hpp>  // for hash

#include "model/table/column_index.h"  // for ColumnIndex
#include "model/table/tuple_index.h"   // for TupleIndex

namespace model {
class TypedColumnData;
}

namespace algos::order {
using Node = std::vector<model::ColumnIndex>;
using AttributeList = std::vector<model::ColumnIndex>;
using Prefixes = std::vector<AttributeList>;
using CandidatePairs = std::vector<std::pair<AttributeList, AttributeList>>;
using ListHash = boost::hash<AttributeList>;
using CandidateSets =
        std::unordered_map<AttributeList, std::unordered_set<AttributeList, ListHash>, ListHash>;
using OrderDependencies =
        std::unordered_map<AttributeList, std::unordered_set<AttributeList, ListHash>, ListHash>;

struct IndexedByteData {
    model::TupleIndex index;
    std::byte const* data;
};

void PrintOD(AttributeList const& lhs, AttributeList const& rhs);
Prefixes GetPrefixes(Node const& node);
AttributeList MaxPrefix(AttributeList const& attribute_list);
bool InUnorderedMap(OrderDependencies const& map, AttributeList const& lhs,
                    AttributeList const& rhs);
bool AreDisjoint(AttributeList const& a, AttributeList const& b);
bool StartsWith(AttributeList const& rhs_candidate, AttributeList const& rhs);
std::unordered_set<model::TupleIndex> GetNullIndices(
        std::vector<model::TypedColumnData> const& data);
std::vector<IndexedByteData> GetIndexedByteData(
        model::TypedColumnData const& data, std::unordered_set<model::TupleIndex> const& null_rows);
}  // namespace algos::order
