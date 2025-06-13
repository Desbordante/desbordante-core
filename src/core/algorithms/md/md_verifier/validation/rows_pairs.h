#pragma once
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

#include "algorithms/md/similarity.h"
#include "model/index.h"

namespace algos::md {
using RowsPair = std::pair<model::Index, model::Index>;
using RowsPairSet = boost::unordered_map<model::Index, boost::unordered_set<model::Index>>;
using RowsToSimilarityMap = boost::unordered_map<RowsPair, model::md::Similarity>;
}  // namespace algos::md