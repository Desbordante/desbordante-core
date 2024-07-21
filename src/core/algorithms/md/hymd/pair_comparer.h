#pragma once

#include <unordered_set>

#include "algorithms/md/hymd/column_match_info.h"
#include "algorithms/md/hymd/compressed_record.h"
#include "algorithms/md/hymd/indexes/records_info.h"
#include "algorithms/md/hymd/pair_comparison_result.h"
#include "algorithms/md/hymd/table_identifiers.h"

namespace algos::hymd {
class PairComparer {
    indexes::RecordsInfo const* const records_info_;
    std::vector<ColumnMatchInfo> const* const column_matches_sim_info_;
    // One table + all sim measures are symmetrical + equality means 1.0
    bool const short_sampling_;
    RecordIdentifier left_record_id_ = 0;
    std::size_t const left_size_ = records_info_->GetLeftCompressor().GetNumberOfRecords();
    std::size_t const column_match_number_ = column_matches_sim_info_->size();

public:
    PairComparer(indexes::RecordsInfo const* records_info,
                 std::vector<ColumnMatchInfo> const* column_matches_sim_info, bool short_sampling)
        : records_info_(records_info),
          column_matches_sim_info_(column_matches_sim_info),
          short_sampling_(short_sampling) {}

    std::unordered_set<PairComparisonResult> SampleNext();

    bool SamplingUnfinished() const noexcept {
        return left_record_id_ != left_size_;
    }

    [[nodiscard]] PairComparisonResult CompareRecords(CompressedRecord const& left_record,
                                                      CompressedRecord const& right_record) const;
};
}  // namespace algos::hymd
