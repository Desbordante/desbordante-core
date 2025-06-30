#include "algorithms/mde/hymde/record_match_indexes/upper_set_index.h"

namespace {
using namespace algos::hymde::record_match_indexes;

UpperSetMapping FillSets(FlatUpperSetIndex const& flat) {
    UpperSetMapping sets;
    auto const& [sorted_records, end_ids] = flat;
    sets.reserve(end_ids.size());
    auto const rec_begin = sorted_records.begin();
    for (auto const& [rcv_id, end_index] : end_ids) {
        sets.try_emplace(sets.end(), rcv_id, rec_begin, rec_begin + end_index);
    }
    return sets;
}
}  // namespace

namespace algos::hymde::record_match_indexes {
FastUpperSetMapping::FastUpperSetMapping(FlatUpperSetIndex flat)
    : flat_(std::move(flat)), sets_(FillSets(flat_)) {}
}  // namespace algos::hymde::record_match_indexes
