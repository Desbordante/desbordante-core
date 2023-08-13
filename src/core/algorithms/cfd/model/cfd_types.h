#pragma once

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>

#include "raw_cfd.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

// Type of row in a transactional table
using Transaction = std::vector<int>;

// Type of one item in a transactional table.
// It is represented as the index of the element with type ItemInfo.
// ItemInfo contains info about one elem in the table.
using Item = int;

// Set of items
using Itemset = std::vector<Item>;

// the set of tids of tuples (indexes of rows in a table) that support concrete Item.
using SimpleTIdList = std::vector<Item>;
using PairHash = boost::hash<std::pair<int, std::string>>;

// Representation of CFD of the form left items -> right item
using ItemsetCFD = std::pair<Itemset, Item>;
using ItemsetCFDList = std::vector<ItemsetCFD>;
using CFDList = std::list<RawCFD>;

// Aliases for frequently used types
using PartitionList = std::vector<std::pair<Itemset, std::vector<unsigned>>>;
using RhsesPair2DList = std::vector<std::vector<std::pair<int, int>>>;
using RhsesPairList = std::vector<std::pair<int, int>>;
using RuleIxs = boost::unordered_map<Itemset, unsigned>;

}  // namespace algos::cfd
