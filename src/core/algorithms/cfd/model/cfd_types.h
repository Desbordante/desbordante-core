#pragma once

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/cfd/model/raw_cfd.h"

namespace algos::cfd {

// Type of row in a transactional table
using Transaction = std::vector<int>;

// Type of one item in a transactional table.
// It is represented as the index of the element with type ItemInfo.
// ItemInfo contains info about one elem in the table.
using Item = int;

// Set of items
using Itemset = std::vector<Item>;

// Representation of CFD of the form left items -> right item
using ItemsetCFD = std::pair<Itemset, Item>;
using CFDList = std::list<RawCFD>;
}  // namespace algos::cfd
