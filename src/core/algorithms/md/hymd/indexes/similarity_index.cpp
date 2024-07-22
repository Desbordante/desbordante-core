#include "algorithms/md/hymd/indexes/similarity_index.h"

#include <ranges>

namespace {
using namespace algos::hymd;
using namespace algos::hymd::indexes;

boost::container::flat_map<ColumnClassifierValueId, RecSet> FillSets(
        FlatUpperSetIndex const& flat) {
    boost::container::flat_map<ColumnClassifierValueId, RecSet> sets;
    auto const& [sorted_records, end_ids] = flat;
    sets.reserve(end_ids.size());
    auto const rec_begin = sorted_records.begin();
    for (auto const& [ccv_id, end_index] : end_ids | std::views::reverse) {
        sets.try_emplace(sets.end(), ccv_id, rec_begin, rec_begin + end_index);
    }
    return sets;
}

boost::container::flat_map<ColumnClassifierValueId, RecSet> PrepareSets(
        FlatUpperSetIndex const& flat) {
    boost::container::flat_map<ColumnClassifierValueId, RecSet> sets;
    auto const& end_ids = flat.end_ids;
    sets.reserve(end_ids.size());
    for (auto const& [ccv_id, end_index] : end_ids | std::views::reverse) {
        sets.try_emplace(sets.end(), ccv_id);
    }
    return sets;
}
}  // namespace

namespace algos::hymd::indexes {
FastUpperSetMapping::FastUpperSetMapping(FlatUpperSetIndex flat)
    : flat_(std::move(flat)), sets_(FillSets(flat_)) {}

CachingUpperSetMapping::CachingUpperSetMapping(FlatUpperSetIndex flat)
    : flat_(std::move(flat)), sets_(PrepareSets(flat_)) {}

}  // namespace algos::hymd::indexes
